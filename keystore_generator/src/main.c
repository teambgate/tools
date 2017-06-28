/*
 * Copyright (C) 2017 Manh Tran
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <smartfox/data.h>
#include <cherry/stdio.h>
#include <cherry/string.h>
#include <cherry/xml/xml.h>
#include <cherry/array.h>
#include <cherry/map.h>
#include <cherry/bytes.h>
#include <cherry/stdlib.h>
#include <cherry/unistd.h>
#include <ftw.h>

#define BUFFER_LEN 1024

int main(int argc, char **argv)
{
        system("mkdir -p res/keystore");

        char line[BUFFER_LEN];
        struct smart_object *in = smart_object_alloc();

        #define ADD_INFO(val, log, key) \
        struct string *val = smart_object_get_string(in, qlkey(key), SMART_GET_REPLACE_IF_WRONG_TYPE); \
        if(val->len == 0) {       \
                memset(line, 0, BUFFER_LEN);    \
                app_log(log);   \
                if(fgets(line, BUFFER_LEN, stdin) != NULL) {    \
                        string_cat(val, line, strlen(line));      \
                        string_trim(val); \
                }       \
        }

        ADD_INFO(alias,         "Enter alias            : ", "alias");
        ADD_INFO(pass,          "Enter password         : ", "pass");
        ADD_INFO(name,          "Enter name             : ", "name");
        ADD_INFO(orgnaization,  "Enter orgnaization     : ", "orgnaization");
        ADD_INFO(country,       "Enter country          : ", "country");

        struct string *command = string_alloc(0);
        string_cat(command, qlkey("keytool -genkey -dname \"cn="));
        string_cat_string(command, name);
        string_cat(command, qlkey(", ou="));
        string_cat_string(command, orgnaization);
        string_cat(command, qlkey(", o="));
        string_cat_string(command, orgnaization);
        string_cat(command, qlkey(", c="));
        string_cat_string(command, country);
        string_cat(command, qlkey("\" -alias "));
        string_cat_string(command, alias);
        string_cat(command, qlkey(" -keypass "));
        string_cat_string(command, pass);
        string_cat(command, qlkey(" -storepass "));
        string_cat_string(command, pass);
        string_cat(command, qlkey(" -keystore res/keystore/key.keystore -keyalg RSA -validity 9999"));
        debug("S: %s\n", command->ptr);
        system(command->ptr);
        string_free(command);
        {
                command = string_alloc(0);
                string_cat(command, qlkey("keytool -export -alias "));
                string_cat_string(command, alias);
                string_cat(command, qlkey(" -storepass "));
                string_cat_string(command, pass);
                string_cat(command, qlkey(" -keystore res/keystore/key.keystore"));
                string_cat(command, qlkey(" | openssl sha1 -binary | openssl base64"));
                debug("S: %s\n", command->ptr);
                
                FILE *fp;
                char path[1035];

                /* Open the command for reading. */
                fp = popen(command->ptr, "r");
                if (fp == NULL) {
                        printf("Failed to run command\n" );
                        exit(1);
                }

                /* Read the output a line at a time - output it. */
                while (fgets(path, sizeof(path)-1, fp) != NULL) {
                        debug("key hash %s\n", path);
                        struct string *val = smart_object_get_string(in, qlkey("keyhash"), SMART_GET_REPLACE_IF_WRONG_TYPE);
                        val->len = 0;
                        string_cat(val, path, strlen(path)-1);
                }

                /* close */
                pclose(fp);
                string_free(command);
        }


        struct string *content = smart_object_to_json(in);

        struct file *f = file_open("res/keystore/info.json", "w", FILE_INNER);
        file_write(f, content->ptr, content->len);
        file_close(f);
        string_free(content);

        smart_object_free(in);
        return 1;
}
