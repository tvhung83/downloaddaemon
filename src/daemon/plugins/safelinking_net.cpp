/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define PLUGIN_CAN_PRECHECK
#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) 
{
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* handle = get_handle();
	string result;
	handle->setopt(CURLOPT_URL, get_url());
	handle->setopt(CURLOPT_HEADER, 1);
	handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
	handle->setopt(CURLOPT_WRITEDATA, &result);
	handle->setopt(CURLOPT_COOKIEFILE, "");
	int res = handle->perform();
	string url = get_url();
	log_string("Safelinking.net: trying to decrypt " + url,LOG_DEBUG);
	if(res != 0)
	{
		log_string("Safelinking.net: handle failed! Please check internet connection or contact safelinking.net",LOG_DEBUG);
		return PLUGIN_CONNECTION_ERROR;
	}
	//string newurl;
	download_container urls;
	//download_container* dlc = get_dl_container();
	//download *download = get_dl_ptr();
	//int dlid = download->get_id();
	try {
		//file deleted?
		if(result.find("(\"This link does not exist.\"|ERROR - this link does not exist)")!= std::string::npos)
		{
			log_string("Safelinking.net: Perhaps wrong URL or the download is not available anymore.",LOG_DEBUG);
			return PLUGIN_FILE_NOT_FOUND;
		}
		if(result.find(">Not yet checked</span>")!= std::string::npos)
		{
			log_string("Safelinking.net: Not yet checked",LOG_DEBUG);
			return PLUGIN_ERROR;
		}
		if(result.find("To use reCAPTCHA you must get an API key from")!= std::string::npos)
		{
			log_string("Safelinking.net: Server error, please contact the safelinking.net support!", LOG_DEBUG);
			set_wait_time(600);
			return PLUGIN_SERVER_OVERLOADED;
		}
		if(url.find("/d/")== std::string::npos)
		{
			string rslt="http://ghost-zero.webs.com/password.png";
			handle->setopt(CURLOPT_HEADER, 0);
			handle->setopt(CURLOPT_FOLLOWLOCATION,0);
			//because sometimes the first time it doesnt work
			for (int i = 0; i <= 5; i++)
			{
				string post = "post-protect=1";
				handle->setopt(CURLOPT_POST, 0);
				if(result.find("type=\"password\" name=\"link-password\"")!= std::string::npos)
				{
					//password protected => ask user for password
					handle->setopt(CURLOPT_URL, rslt);
					string newresult = result;
					result.clear();
					handle->perform();
					std::string captcha_text = Captcha.process_image(result, "png", "", -1, false, false, captcha::SOLVE_MANUAL);
					post += "&link-password=" + captcha_text;
					result = newresult;
					log_string("Safelinking.net: password = " + captcha_text,LOG_DEBUG);
				}
				if(result.find("api.recaptcha.net")!= std::string::npos)
				{
					//recaptcha!!!
					log_string("Safelinking.net: Using recaptcha",LOG_DEBUG);
					size_t urlpos = result.find("<iframe src=\"http://api.recaptcha.net/noscript?k=");
					if(urlpos == string::npos)
					{
						log_string("Recaptcha1: Safelinking.net: urlpos is at end of file",LOG_DEBUG);
						return PLUGIN_ERROR;
					}
					urlpos += 49;
					string id = result.substr(urlpos, result.find("\"", urlpos) - urlpos);
					replace_all(id,"&amp;error=1","");
					log_string("Safelinking.net: id = : " + id, LOG_DEBUG);
					if(id == "")
					{
						log_string("Safelinking.net: reCaptcha ID or div couldn't be found...",LOG_DEBUG);
						return PLUGIN_ERROR;
					}
					else
					{
						handle->setopt(CURLOPT_URL, "http://api.recaptcha.net/challenge?k=" + id);
						/* follow redirect needed as google redirects to another domain */
						handle->setopt(CURLOPT_FOLLOWLOCATION,1);
						handle->setopt(CURLOPT_POST, 0);
						result.clear();
						handle->perform();
						//log_string("Safelinking.net = "+result,LOG_DEBUG);
						size_t urlpos = result.find("challenge : '");
						if(urlpos == string::npos)
						{
							log_string("Recaptcha2: Safelinking.net: urlpos is at end of file",LOG_DEBUG);
							return PLUGIN_ERROR;
						}
						urlpos += 13;
						string challenge = result.substr(urlpos, result.find("',", urlpos) - urlpos);
						urlpos = result.find("server : '");
						if(urlpos == string::npos)
						{
							log_string("Recaptcha3: Safelinking.net: urlpos is at end of file",LOG_DEBUG);
							return PLUGIN_ERROR;
						}
						urlpos += 10;
						string server = result.substr(urlpos, result.find("',", urlpos) - urlpos);
						if(challenge == "" || server == "")
						{
							log_string("Safelinking.net: Recaptcha Module fails: " + url,LOG_DEBUG);
							return PLUGIN_ERROR;
						}
						string captchaAddress = server + "image?c=" + challenge;
						log_string("Safelinking.net: captchastring=" + captchaAddress,LOG_DEBUG);
						handle->setopt(CURLOPT_URL, captchaAddress);
						result.clear();
						handle->perform();
						std::string captcha_text = Captcha.process_image(result, "jpg", "", -1, false, false, captcha::SOLVE_MANUAL);
						post += "&recaptcha_challenge_field=" + challenge;
						post += "&recaptcha_response_field=" + captcha_text;
					}
				}
				else if(size_t urlpos=result.find("http://safelinking.net/includes/captcha_factory/securimage/securimage_show.php?sid=")!= std::string::npos)
				{
					string captchaAddress = result.substr(urlpos, result.find("\"", urlpos) - urlpos);
					handle->setopt(CURLOPT_URL, captchaAddress);
					result.clear();
					handle->perform();
					std::string captcha_text = Captcha.process_image(result, "jpg", "", -1, false, false, captcha::SOLVE_MANUAL);
					post+= "&securimage_response_field=" + captcha_text;
				}
				else if(size_t urlpos=result.find("http://safelinking.net/includes/captcha_factory/3dcaptcha/3DCaptcha.php")!= std::string::npos)
				{
					string captchaAddress = result.substr(urlpos, result.find("\"", urlpos) - urlpos);
					handle->setopt(CURLOPT_URL, captchaAddress);
					result.clear();
					handle->perform();
					std::string captcha_text = Captcha.process_image(result, "jpg", "", -1, false, false, captcha::SOLVE_MANUAL);
					post+= "&3dcaptcha_response_field=" + captcha_text;
				}
				else if(result.find("class=\"captcha_image ajax-fc-container\"")!= std::string::npos)
				{
					log_string("Safelinking.net: fancycaptcha!",LOG_DEBUG);
					handle->setopt(CURLOPT_URL, "http://safelinking.net/includes/captcha_factory/fancycaptcha.php?hash=" + search_between(url,"safelinking.net/p/"));
					result.clear();
					handle->perform();
					post+= "&fancy-captcha=" + trim_string(result);
				}
				handle->setopt(CURLOPT_COPYPOSTFIELDS, post);
				handle->setopt(CURLOPT_URL, get_url());
				handle->setopt(CURLOPT_POST, 1);
				result.clear();
				handle->perform();
				if (result.find("api.recaptcha.net")!= std::string::npos ||
				    result.find("http://safelinking.net/includes/captcha_factory/securimage/securimage_show.php?sid=")!= std::string::npos ||
				    result.find("http://safelinking.net/includes/captcha_factory/3dcaptcha/3DCaptcha.php")!= std::string::npos ||
				    result.find("type=\"password\" name=\"link-password\"")!= std::string::npos)
				{
					post.clear();
					continue;
				}
				if(result.find("fancycaptcha.css\"")!= std::string::npos)
				{
					log_string("Safelinking.net: failed with fancycaptcha = " + url, LOG_DEBUG);
					return PLUGIN_ERROR;
				}
				break;
			}
			if (result.find("api.recaptcha.net")!= std::string::npos ||
			    result.find("http://safelinking.net/includes/captcha_factory/securimage/securimage_show.php?sid=")!= std::string::npos ||
			    result.find("http://safelinking.net/includes/captcha_factory/3dcaptcha/3DCaptcha.php")!= std::string::npos ||
			    result.find("type=\"password\" name=\"link-password\"")!= std::string::npos)
			{
				log_string("Safelinking.net: failed after 6 times = " + url, LOG_DEBUG);
				return PLUGIN_ERROR;
			}
			if(result.find(">All links are dead.<")!= std::string::npos)
			{
				log_string("Safelinking.net: Perhaps wrong URL or the download is not available anymore.",LOG_DEBUG);
				return PLUGIN_FILE_NOT_FOUND;
			}
			log_string("so far, so good!\ngetting direct links",LOG_DEBUG);
			vector<string> links;
			size_t urlpos = result.find("class=\"link-box\" id=\"direct-links\"");
			if(urlpos == string::npos)
			{
				log_string("direct links1: Safelinking.net: urlpos is at end of file",LOG_DEBUG);
				links = search_all_between(result,"class=\"linked\">","</a>",0,false);
				if(links.size()==0)
				{
					log_string("direct links2: Safelinking.net: urlpos is at end of file",LOG_DEBUG);
					string lnks = search_between(result,"class=\"result-form\">","</fieldset></div></div></div></div><div id=\"footer\"");
					if(lnks!="")
					{
						links = search_all_between(lnks,"href=\"","\"",0,false);
					}
					else
					{
						log_string("direct links3: Safelinking.net: urlpos is at end of file",LOG_DEBUG);
						return PLUGIN_ERROR;
					}
				}
				
			}
			else
			{
				links = search_all_between(result,"<a href=\"","\"",0,true);
			}
			if(links.size()==0)
			{
				return PLUGIN_FILE_NOT_FOUND;
			}
			
			for(size_t i = 0; i < links.size(); i++)
			{
				links[i] = trim_string(links[i]);
				urls.add_download(links[i], "");
			}
		}
		else
		{
			//just a simple redirect
			log_string("simple redirect!",LOG_DEBUG);
			size_t urlpos = result.find("Location:");
			if(urlpos == string::npos)
			{
				log_string("/d: Safelinking.net: urlpos is at end of file",LOG_DEBUG);
				return PLUGIN_ERROR;
			}
			urlpos += 10;
			string newurl = result.substr(urlpos, result.find_first_of("\r\n", urlpos) - urlpos);
			urls.add_download(set_correct_url(newurl),"");
		}
	} catch(...) {}
	replace_this_download(urls);
	//dlc->add_download(set_correct_url(newurl), "");
	//dlc->set_status(dlid, DOWNLOAD_DELETED);
	//set_url(set_correct_url(newurl));
	return PLUGIN_SUCCESS;
}

bool get_file_status(plugin_input &inp, plugin_output &outp) 
{
	return false;
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) 
{
	outp.allows_resumption = false;
	outp.allows_multiple = false;

	outp.offers_premium = false;
}
