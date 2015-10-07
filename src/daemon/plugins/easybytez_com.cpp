/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "plugin_helpers.h"
#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
using namespace std;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
	std::string *blubb = (std::string*)userp;
	blubb->append((char*)buffer, nmemb);
	return nmemb;
}

//Only Free-User support
plugin_status plugin_exec(plugin_input &inp, plugin_output &outp) {
	ddcurl* handle = get_handle();
	std::string resultstr;
	string url = get_url();
    vector<string> splitted_url = split_string(url,"/");

    if (url.find("users") == string::npos) {
        handle->setopt(CURLOPT_POST,0);
        handle->setopt(CURLOPT_URL, url.c_str());
        handle->setopt(CURLOPT_COOKIEFILE, "");
        handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
        handle->setopt(CURLOPT_WRITEDATA, &resultstr);

        int success = handle->perform();

        if(success != 0) {
            return PLUGIN_CONNECTION_ERROR;
        }
        vector<string> post_strings1 = search_all_between(resultstr, "input type=\"hidden\" name=", ">");
        string post_string;
        post_string.clear();
        for(size_t i = 0; i < post_strings1.size(); ++i) {
            vector<string> name_value = search_all_between(post_strings1[i], "\"", "\"");
            post_string += name_value[0] + "=" + name_value[1] +"&";
        }
        post_string += "method_free=Free+Download";
        resultstr.clear();
        handle->setopt(CURLOPT_URL, url.c_str());
        handle->setopt(CURLOPT_REFERER, url.c_str());
        handle->setopt(CURLOPT_POST, 1);
        handle->setopt(CURLOPT_COPYPOSTFIELDS, post_string);
        handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
        handle->setopt(CURLOPT_WRITEDATA, &resultstr);
        handle->perform();
        if (resultstr.find("<div class=\"err\">You have to wait ") != string::npos){
            string wait_time=search_between(resultstr,"<div class=\"err\">You have to wait "," seconds till next download");
            int Minutes=atoi(wait_time.substr(0,wait_time.find(" ")).c_str())*60;
            int Seconds=atoi(wait_time.substr(wait_time.find(", ")+2,wait_time.length()-wait_time.find(", ")+2).c_str())+2;
            set_wait_time(Minutes+Seconds);
            return PLUGIN_LIMIT_REACHED;
        }
        string link_wait_time = search_between(resultstr, "<span id=\"countdown_str\">Wait","</span> seconds</span>");
        link_wait_time = search_between(link_wait_time,">","<");
        set_wait_time(atoi(link_wait_time.c_str()));
        sleep(atoi(link_wait_time.c_str()));
        vector<string> post_strings2 = search_all_between(resultstr, "input type=\"hidden\" name=", ">");
        post_string.clear();
        for(size_t i = 0; i < post_strings2.size(); ++i) {
            vector<string> name_value = search_all_between(post_strings2[i], "\"", "\"");
            post_string += name_value[0] + "=" + name_value[1] +"&";
        }
        post_string += "btn_download=Download+File";
        resultstr.clear();

        handle->setopt(CURLOPT_URL, url.c_str());
        handle->setopt(CURLOPT_REFERER, url.c_str());
        handle->setopt(CURLOPT_FOLLOWLOCATION, 0);
        handle->setopt(CURLOPT_POST, 1);
        handle->setopt(CURLOPT_HEADER, 1);
        handle->setopt(CURLOPT_COPYPOSTFIELDS, post_string);
        handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
        handle->setopt(CURLOPT_WRITEDATA, &resultstr);
        handle->perform();

        handle->setopt(CURLOPT_POST, 0);
        handle->setopt(CURLOPT_HEADER, 0);

        outp.download_url = search_between(resultstr, "Location: ", "\r\n");
        return PLUGIN_SUCCESS;
    }
    else
    {
        if (splitted_url.size() > 6){
            download_container urls;
            log_string("easybytez: receive Folder Links", LOG_DEBUG);
            vector<string> links;
            string new_url=url.c_str();
            do{
                resultstr.clear();
                handle->setopt(CURLOPT_POST,0);
                handle->setopt(CURLOPT_URL, new_url);
                handle->setopt(CURLOPT_COOKIEFILE, "");
                handle->setopt(CURLOPT_WRITEFUNCTION, write_data);
                handle->setopt(CURLOPT_WRITEDATA, &resultstr);
                handle->perform();
                links = search_all_between(resultstr, "<div class=\"link\"><a href=\"","\" target=\"_blank\">");
                for(size_t i = 0; i < links.size(); i++) {
                    log_string("easybytez: Add Link:" + links[i], LOG_DEBUG);
                    urls.add_download(links[i], "");
                }
                size_t Found_Next=resultstr.find("'>Next ");
                size_t Found_Link=resultstr.rfind("<a href='", Found_Next);
                string page=resultstr.substr(Found_Link+9, Found_Next-Found_Link-9);
                new_url=url.c_str()+page;

            } while(resultstr.find("'>Next ") != string::npos);
            replace_this_download(urls);
            return PLUGIN_SUCCESS;
        }
        else
            log_string("easybytez: ERROR Receiving folder-content" , LOG_DEBUG);
            return PLUGIN_ERROR;
   }
}

extern "C" void plugin_getinfo(plugin_input &inp, plugin_output &outp) {
	if(!inp.premium_user.empty() && !inp.premium_password.empty()) {
		outp.allows_resumption = true;
		outp.allows_multiple = true;
	} else {
		outp.allows_resumption = false;
        outp.allows_multiple = false;
	}
	outp.offers_premium = false;
}
