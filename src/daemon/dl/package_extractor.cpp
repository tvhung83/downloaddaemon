/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "package_extractor.h"
#include "../global.h"
#include "../tools/helperfunctions.h"

#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
using std::string;

//declare class static objects
std::string pkg_extractor::last_posted_message;
int pkg_extractor::container_id;
std::recursive_mutex pkg_extractor::mx;

pkg_extractor::extract_status pkg_extractor::extract_package(const std::string& filename, const int& id, const std::string& password) {
	pkg_extractor::container_id = id;
	tool to_use = required_tool(filename);
	pkg_extractor::extract_status extract_status;
	switch(to_use) {
		case GNU_UNRAR:
		case RARLAB_UNRAR:
			extract_status =  extract_rar(filename, password, to_use);
			break;
		case TAR:
		case TAR_BZ2:
		case TAR_GZ:
			extract_status = extract_tar(filename, to_use);
			break;
		case ZIP:
			extract_status = extract_zip(filename, password, to_use);
			break;
		case HJSPLIT:
			extract_status = merge_hjsplit(filename, to_use);
			break;
		case SEVENZ:
			extract_status = extract_7z(filename,password,to_use);
			break;
		default:
			return PKG_INVALID;
	}
	string targetDir = getTargetDir(filename,to_use);
	extract_status = deep_extract(targetDir,extract_status,password);
	return extract_status;
}

pkg_extractor::extract_status pkg_extractor::deep_extract(const std::string& targetDir,pkg_extractor::extract_status extract_status, const std::string& password)
{
	//deep extract	
	log_string("Deep unrar: target dir=" + targetDir,LOG_DEBUG);
	std::vector<std::string> files = getDir(targetDir);
	if(!files.empty())
	{
		for(size_t i = 0; i < files.size(); i++)
		{
			log_string("Trying to deep extract "+ files[i],LOG_DEBUG);
			std::vector<std::string> splitted = split_string(files[i],"/");
			if(opendir(files[i].c_str()) != NULL && splitted.back()!="." && splitted.back()!="..")
				deep_extract(files[i],extract_status,password);
			pkg_extractor::extract_status temp = extract_package(files[i], container_id, password);
			if(temp != PKG_INVALID)
				extract_status = temp;
		}
	}
	return extract_status;
}

pkg_extractor::tool pkg_extractor::required_tool(std::string filename) {
	size_t n = filename.find_last_of("\\/");
	if(n != string::npos) {
		filename = filename.substr(n + 1);
	}

	// we also have to check for string::npos, because size() - x could be -1, which equals string::npos
	if((n = filename.find(".rar")) == filename.size() - 4 && n != string::npos) {
		return get_unrar_type();
	}

	if((n = filename.find(".tar")) == filename.size() - 4 && n != string::npos) {
		return TAR;
	}

	if((n = filename.find(".tar.bz2")) == filename.size() - 8 && n != string::npos) {
		return TAR_BZ2;
	}

	if((n = filename.find(".tar.gz")) == filename.size() - 7 && n != string::npos) {
		return TAR_GZ;
	}

	if((n = filename.find(".zip")) == filename.size() - 4 && n != string::npos) {
		return ZIP;
	}

	if((n = filename.find(".001")) == filename.size() - 4 && n != string::npos) {
		return HJSPLIT;
	}

	if((n = filename.find(".7z")) == filename.size() - 3 && n != string::npos) {
		return SEVENZ;
	}

	return NONE;
}

pkg_extractor::tool pkg_extractor::get_unrar_type() {
	std::string unrar_path_s = global_config.get_cfg_value("unrar_path");
	if(unrar_path_s.empty()) return NONE;

	pid_t child_id;
	int pipe_conn[2];
	if(pipe(pipe_conn) != 0) {
		log_string("Failed to open pipe while trying to detect your unrar version!", LOG_ERR);
		return NONE;
	}

	string help_result;
	char buf[256];
	char unrar_path[FILENAME_MAX];
	strncpy(unrar_path, unrar_path_s.c_str(), FILENAME_MAX);

	if((child_id = fork()) == 0) {
		// child process
		// we only need to write to the pipe, not read
		dup2(pipe_conn[1], 1);
		dup2(pipe_conn[1], 2);
		close(pipe_conn[0]);
		execlp(unrar_path, unrar_path, "--help", NULL);

	} else {
		// parent process
		dup2(pipe_conn[0], 0);
		close(pipe_conn[1]);

		size_t num;
		while((num = read(pipe_conn[0], buf, 256)) > 0) {
			help_result.append(buf, num);
		}
		int ret;
		waitpid(child_id, &ret, 0);
		if(help_result.empty()) return NONE;

		if(help_result.find("  p-") != string::npos) {
			return RARLAB_UNRAR;
		} else {
			return GNU_UNRAR;
		}

	}
	return NONE;
}

pkg_extractor::extract_status pkg_extractor::extract_rar(const std::string& filename, const std::string& password, tool t) {
	std::string unrar_path_s = global_config.get_cfg_value("unrar_path");
	if(unrar_path_s.empty()) return PKG_ERROR;
	pid_t child_id;
	char buf[256];
	bool have_pw = !password.empty();

	char unrar_path[FILENAME_MAX];
	strncpy(unrar_path, unrar_path_s.c_str(), FILENAME_MAX);
	char fn_path[FILENAME_MAX];
	strncpy(fn_path, filename.c_str(), FILENAME_MAX);
	char target_dir[FILENAME_MAX];
	strncpy(target_dir, getTargetDir(filename,t).c_str(), FILENAME_MAX);
	char unrar_pw[512] = "-p";
	strncpy(unrar_pw + 2, password.c_str(), 510);

	unrar_path[FILENAME_MAX - 1] = '\0';
	fn_path[FILENAME_MAX - 1] = '\0';
	target_dir[FILENAME_MAX - 1] = '\0';
	unrar_pw[511] = '\0';

	int ctop[2];

	if(pipe(ctop) != 0) {
		log_string("Failed to open pipe while trying to extract a rar-package", LOG_ERR);
		return PKG_ERROR;
	}
	mkdir_recursive(target_dir);
	if((child_id = fork()) == 0) {
		// child
		close(ctop[0]);

		dup2(ctop[1], STDOUT_FILENO);
		dup2(ctop[1], STDERR_FILENO);
		
		
		if(have_pw) {
			execlp(unrar_path, unrar_path, "x", unrar_pw, "-o+", fn_path, target_dir, NULL);
		} else {
			if(t == RARLAB_UNRAR)
				execlp(unrar_path, unrar_path, "x", "-p-", "-o+", fn_path, target_dir, NULL);
			else
				execlp(unrar_path, unrar_path, "x", "-o+", fn_path, target_dir, NULL);
		}
		write(ctop[1], "ERROR", 5);
		exit(-1);
	} else {
		// parent
		close(ctop[1]);
		post_subscribers(connection_manager::UNRAR_START);
		std::string result;
		size_t num;
		while((num = read(ctop[0], buf, 256)) > 0) {
			result.append(buf, num);
			if((t == RARLAB_UNRAR && result.find("Enter password") != string::npos) || (t == GNU_UNRAR && result.find("Password:") != string::npos)) {
				kill(child_id, 9);
				break;
			}
			if(t == RARLAB_UNRAR && result.find("Reenter password") != string::npos) {
				kill(child_id, 9);
				break;
			}
		}
		int process_ret;
		waitpid(child_id, &process_ret, 0);
		// parse result
		extract_status ret = PKG_SUCCESS;
		if(t == GNU_UNRAR && result.find(" Failed") != string::npos) {
			ret = PKG_PASSWORD;
		} else if(t == RARLAB_UNRAR && (result.find("password incorrect") != string::npos ))
		{
			post_subscribers(connection_manager::UNRAR_FINISHED,"password incorrect");
			return PKG_PASSWORD;
		}
		else if (t == RARLAB_UNRAR && (result.find("No files to extract") != string::npos))
		{
			post_subscribers(connection_manager::UNRAR_FINISHED,"password needed");
			return PKG_PASSWORD;
		}

		if(process_ret != 0 || result.find("ERROR") != string::npos  || (t == RARLAB_UNRAR && result.find("All OK") == string::npos)) 
		{
			size_t s;
			std::string temp;
            if((s = result.find("already exists")) != string::npos)
			{
				post_subscribers(connection_manager::UNRAR_FINISHED,"ERROR already exist");
			}
            s = 0;
            while((s = result.find("CRC failed in volume",s)) != string::npos)
			{
                s += 20;
                temp = result.substr(s,result.find("\n",s) - s);
				post_subscribers(connection_manager::UNRAR_FINISHED,"ERROR " + temp);
			}
			ret = PKG_ERROR;
		}
        if(ret == PKG_SUCCESS && result.find("All OK") !=  string::npos)
			post_subscribers(connection_manager::UNRAR_FINISHED,"sucessfully extracted");

		return ret;
	}

	return PKG_ERROR;
}

pkg_extractor::extract_status pkg_extractor::extract_tar(const std::string& filename, tool t) {
	std::string tar_path_s = global_config.get_cfg_value("tar_path");
	if(tar_path_s.empty()) return PKG_ERROR;
	pid_t child_id;
	char buf[256];

	char tar_path[FILENAME_MAX];
	strncpy(tar_path, tar_path_s.c_str(), FILENAME_MAX);
	char fn_path[FILENAME_MAX];
	strncpy(fn_path, filename.c_str(), FILENAME_MAX);
	char target_dir[FILENAME_MAX];

	strncpy(target_dir, getTargetDir(filename,t).c_str(), FILENAME_MAX);

	tar_path[FILENAME_MAX - 1] = '\0';
	fn_path[FILENAME_MAX - 1] = '\0';
	target_dir[FILENAME_MAX - 1] = '\0';

	int ctop[2];
	if(pipe(ctop) != 0) {
		log_string("Failed to open pipe while trying to extract a tar-package", LOG_ERR);
		return PKG_ERROR;
	}
	mkdir_recursive(target_dir);
	if((child_id = fork()) == 0) {
		close(ctop[0]);
		dup2(ctop[1], STDOUT_FILENO);
		dup2(ctop[1], STDERR_FILENO);

		switch(t) {
			case TAR:
				execlp(tar_path, tar_path, "xf", fn_path, "-C", target_dir, NULL);
			exit(-1);
			case TAR_GZ:
				execlp(tar_path, tar_path, "xzf", fn_path, "-C", target_dir, NULL);
			exit(-1);
			case TAR_BZ2:
				execlp(tar_path, tar_path, "xjf", fn_path, "-C", target_dir, NULL);
			exit(-1);
			default:
			exit(-1);
		}
	} else {
		// parent
		close(ctop[1]);

		std::string result;
		size_t num;
		// we don't need the output. we just ignore it.
		while((num = read(ctop[0], buf, 256)) > 0);

		int retval;
		waitpid(child_id, &retval, 0);
		if(retval == 0) {
			return PKG_SUCCESS;
		}
	}
	return PKG_ERROR;
}

pkg_extractor::extract_status pkg_extractor::extract_zip(const std::string& filename, const std::string& password, tool t) {
	//std::string unzip_path_s = "/usr/bin/unzip";
 	std::string unzip_path_s = global_config.get_cfg_value("unzip_path");
	if(unzip_path_s.empty()) return PKG_ERROR;
	pid_t child_id;
	char buf[256];

	char unzip_path[FILENAME_MAX];
	strncpy(unzip_path, unzip_path_s.c_str(), FILENAME_MAX);
	char fn_path[FILENAME_MAX];
	strncpy(fn_path, filename.c_str(), FILENAME_MAX);
	char target_dir[FILENAME_MAX];
	char pass[512];
	strncpy(pass, password.c_str(), 512);

	strncpy(target_dir, getTargetDir(filename,t).c_str(), FILENAME_MAX);

	unzip_path[FILENAME_MAX - 1] = '\0';
	fn_path[FILENAME_MAX - 1] = '\0';
	target_dir[FILENAME_MAX - 1] = '\0';
	pass[511] = '\0';

	bool have_pw = !password.empty();
	mkdir_recursive(target_dir);

	int ctop[2];
	if(pipe(ctop) != 0) {
		//log_string("Failed to open pipe while trying to extract a tar-package", LOG_ERR);
		return PKG_ERROR;
	}
	if((child_id = fork()) == 0) {
		close(ctop[0]);
		dup2(ctop[1], STDOUT_FILENO);
		dup2(ctop[1], STDERR_FILENO);

		if(have_pw) {
			execlp(unzip_path, unzip_path, "-o", "-P", pass, fn_path, "-d", target_dir, NULL);
		} else {
			execlp(unzip_path, unzip_path, "-o", fn_path, "-d", target_dir, NULL);
		}
		exit(-1);

	} else {
		// parent
		close(ctop[1]);

		std::string result;
		size_t num;
		// we don't need the output. we just ignore it.
		while((num = read(ctop[0], buf, 256)) > 0) {
			result.append(buf, num);
			if(result.find("password:") != string::npos || result.find("incorrect password") != string::npos) {
				kill(child_id, 9);
				return PKG_PASSWORD;
			}
		}

		int retval;
		waitpid(child_id, &retval, 0);
		if(retval == 0) {
			return PKG_SUCCESS;
		}

	}
	return PKG_ERROR;
}

pkg_extractor::extract_status pkg_extractor::merge_hjsplit(const std::string& filename, tool t)
{
	std::string hjsplit_path_s = global_config.get_cfg_value("hjsplit_path");
	if(hjsplit_path_s.empty()) return PKG_ERROR;
	pid_t child_id;
	char buf[256];

	char hjsplit_path[FILENAME_MAX];
	strncpy(hjsplit_path, hjsplit_path_s.c_str(), FILENAME_MAX);
	char fn_path[FILENAME_MAX];
	strncpy(fn_path, filename.c_str(), FILENAME_MAX);
	char target_dir[FILENAME_MAX];

	strncpy(target_dir, getTargetDir(filename,t).c_str(), FILENAME_MAX);

	hjsplit_path[FILENAME_MAX - 1] = '\0';
	fn_path[FILENAME_MAX - 1] = '\0';
	target_dir[FILENAME_MAX - 1] = '\0';

	int ctop[2];
	if(pipe(ctop) != 0) {
		log_string("Failed to open pipe while trying to merge a hjsplit-package", LOG_ERR);
		return PKG_ERROR;
	}
	mkdir_recursive(target_dir);
	if((child_id = fork()) == 0) {
		close(ctop[0]);
		dup2(ctop[1], STDOUT_FILENO);
		dup2(ctop[1], STDERR_FILENO);
		// NO LOGGIN HERE! any complex operation in a forked child in a multithreaded application is VERY
		// dangerous. This inclues simple stuff, like using STL containers or C standard functions
		// log_string("excuting hjsplit path:" + hjsplit_path_s + "\nfn_path:" + filename, LOG_DEBUG);
		chdir(target_dir);
		execlp(hjsplit_path, hjsplit_path, "-j", fn_path, NULL);

		exit(-1);
	} else {
		// parent
		close(ctop[1]);

		std::string result;
		size_t num;
		// we don't need the output. we just ignore it.
		while((num = read(ctop[0], buf, 256)) > 0)
		{
			result.append(buf, num);
		}
		//log_string("result = "+result,LOG_DEBUG);
		int retval;
		waitpid(child_id, &retval, 0);
		if(retval == 0) {
			log_string("HJSPLIT: package merged succesfully",LOG_DEBUG);
			return PKG_SUCCESS;
		}
	}
	return PKG_ERROR;
}

string pkg_extractor::getTargetDir(const std::string &filename, tool t)
{
	std::string unrar_dir = global_config.get_cfg_value("unrar_dir");
	size_t ext_len=0;
	switch(t)
	{
		case HJSPLIT:
		case SEVENZ: ext_len=3; break;
		case GNU_UNRAR:
		case RARLAB_UNRAR:
		case ZIP:
		case TAR: ext_len = 4; break;
		case TAR_GZ: ext_len = 7; break;
		case TAR_BZ2: ext_len = 8; break;
		default: ext_len = 0;
	}
	if(unrar_dir.empty() || unrar_dir == "")
	{
		return filename.substr(0, filename.size() - ext_len);
	}
	else
	{
		std::vector<std::string> file = split_string(filename.substr(0, filename.size() - ext_len),"/");
		log_string("unrar_dir=" + unrar_dir + "/" + file[file.size()-1],LOG_DEBUG);
		return unrar_dir + "/" + file[file.size()-1];
	}
		
}

std::vector<std::string> pkg_extractor::getDir(std::string dir)
{
	DIR *dp;
	std::vector<std::string> files = std::vector<std::string>();
	struct dirent *dirp;
	if((dp  = opendir(dir.c_str())) == NULL) {
		log_string("cannot open dir" + dir, LOG_WARNING);
		return files;
	}

	while ((dirp = readdir(dp)) != NULL) {
		files.push_back(dir + "/" + string(dirp->d_name));
	}
	closedir(dp);
	return files;
}
pkg_extractor::extract_status pkg_extractor::extract_7z(const std::string& filename, const std::string& password, tool t) {
	std::string zip7_path_s = global_config.get_cfg_value("7z_path");
	if(zip7_path_s.empty()) return PKG_ERROR;
	pid_t child_id;
	char buf[256];

	char zip7_path[FILENAME_MAX];
	strncpy(zip7_path, zip7_path_s.c_str(), FILENAME_MAX);
	char fn_path[FILENAME_MAX];
	strncpy(fn_path, filename.c_str(), FILENAME_MAX);
	char target_dir_app[FILENAME_MAX] = "-o";
	char target_dir[FILENAME_MAX];
	char pass[512] = "-p";
	strncpy(pass + 2, password.c_str(), 510);

	strncpy(target_dir_app + 2, getTargetDir(filename,t).c_str(), FILENAME_MAX-2);
	strncpy(target_dir, getTargetDir(filename,t).c_str(), FILENAME_MAX);
	log_string("target_dir = " + string(target_dir),LOG_DEBUG);

	zip7_path[FILENAME_MAX - 1] = '\0';
	fn_path[FILENAME_MAX - 1] = '\0';
	target_dir[FILENAME_MAX - 1] = '\0';
	pass[511] = '\0';

	bool have_pw = !password.empty();
	mkdir_recursive(target_dir);

	int ctop[2];
	if(pipe(ctop) != 0) {
		log_string("Failed to open pipe while trying to extract a 7z-package", LOG_ERR);
		return PKG_ERROR;
	}
	if((child_id = fork()) == 0) {
		close(ctop[0]);
		dup2(ctop[1], STDOUT_FILENO);
		dup2(ctop[1], STDERR_FILENO);

		if(have_pw) {
			execlp(zip7_path, zip7_path, "x", target_dir_app, pass, fn_path, NULL);
		} else {
			execlp(zip7_path, zip7_path, "x", target_dir_app, fn_path, NULL);
		}
		exit(-1);

	} else {
		// parent
		close(ctop[1]);

		std::string result;
		size_t num;
		while((num = read(ctop[0], buf, 256)) > 0) {
			result.append(buf, num);
			if(result.find("password:") != string::npos || result.find("incorrect password") != string::npos) {
				kill(child_id, 9);
				return PKG_PASSWORD;
			}
		}
		log_string("result = " + result,LOG_DEBUG);
		int retval;
		waitpid(child_id, &retval, 0);
		//log_string("retval=" + int_to_string(retval),LOG_DEBUG);
		if(retval == 0) {
			log_string("7zip: package extracted succesfully",LOG_DEBUG);
			return PKG_SUCCESS;
		}

	}
	return PKG_ERROR;
}

void pkg_extractor::post_subscribers(connection_manager::reason_type reason, std::string temp) {
	std::unique_lock<std::recursive_mutex> lock(pkg_extractor::mx);
	std::string line, reason_str;

	connection_manager::reason_to_string(reason, reason_str);
	line = "PACKAGE|" + int_to_string(container_id);
	line = reason_str + ":" + line + ":" + temp;

	if((line != getLastPostMessage()) || (reason == connection_manager::UNRAR_START) || (reason == connection_manager::UNRAR_FINISHED)) {
		connection_manager::instance()->push_message(connection_manager::SUBS_DOWNLOADS, line);
		setLastPostedMessage(line);
	}
}

void pkg_extractor::setLastPostedMessage(std::string message)
{
	last_posted_message = message;
}

std::string pkg_extractor::getLastPostMessage()
{
	return last_posted_message;
}

