/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ddclient_gui_update_thread.h"

update_thread::update_thread(ddclient_gui *parent, int interval) : parent(parent), subscription_enabled(false), told(false),
    update(true), interval(interval), term(false){
}


void update_thread::run(){
	//QMutexLocker lock(parent->get_mutex()); todo removed this because I just forgot why I did it
    // first update
    if(!(parent->check_connection(false)) && (told == false)){ // connection failed for the first time => we're calling get_content to update the gui
		parent->get_content(true);
        told = true;
    }else if(parent->check_connection(false)){ // connection is valid
		parent->get_content(true);
        told = false;
    }

    parent->clear_last_error_message();


    // check if downloaddaemon supports subscription
    subscription_enabled = parent->check_subscritpion();

    // behaviour with subscriptions => less traffic and cpu usage
    if(subscription_enabled){
        while(true){

            if(term)
                return;

            if(!(parent->check_connection(false)) && (told == false)){ // connection failed for the first time => we're calling get_content to update the gui
				parent->get_content(true); // called to clear list
                told = true;

                sleep(1); // otherwise we get 100% cpu if there is no connection

            }else if(parent->check_connection(false)){ // connection is valid
				// possibility to pause updates for a while
				while(!update && !term)
					sleep(1);

				// todo same here, don't know why lock.unlock();
				parent->get_content(); // get updates
				// todo same here, don't know why lock.relock();
                told = false;
            }

            parent->clear_last_error_message();
        }
    }
}


void update_thread::set_update_interval(int interval){
    this->interval = interval;
}


int update_thread::get_update_interval(){
    return interval;
}


void update_thread::set_updating(bool updating){
	update = updating;
}


void update_thread::terminate_yourself(){
    term = true;
}
