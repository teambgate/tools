#include <cherry/stdio.h>
#include <cherry/stdlib.h>
#include <cherry/unistd.h>
#include <cherry/memory.h>
#include <cherry/string.h>
#include <cherry/array.h>
#include <cherry/map.h>
#include <cherry/bytes.h>
#include <cherry/math/math.h>
#include <curl/curl.h>
#include <smartfox/data.h>

#include <arpa/inet.h>

static int is_ip(char *s)
{
        struct sockaddr_in sa;
        int result = inet_pton(AF_INET, s, &(sa.sin_addr));
        return result != 0;
}

static size_t func(void *ptr, size_t size, size_t nmemb, void *d)
{
        object_cast(data, d);
        size_t realsize = size * nmemb;

        object_get_string(server_primary_host, data, qlkey("server_primary_host"));
        object_get_string(server_backup_host, data, qlkey("server_backup_host"));

        server_primary_host->len = 0;
        server_backup_host->len = 0;

        string_cat(server_primary_host, ptr, realsize - 1);
        string_cat(server_backup_host, ptr, realsize - 1);

        if(is_ip(server_primary_host->ptr)) {
                debug("%s\n", ptr);
                struct string *content = smart_object_to_json(data);

                struct file *file = file_open("/cygdrive/d/manh_drive/bigo69/bigo_local.json", "w", FILE_INNER);
                file_write(file, content->ptr, content->len);
                file_close(file);

                string_free(content);
        } else {
                debug("result not ip address\n", ptr);
        }

        return size * nmemb;
}

static void _test(int t)
{
        int i = 0;
check:;
        autofree struct string *temp = NULL;
        temp = string_alloc(0);
        string_cat(temp, qlkey("loop"));
        debug("%s\n", temp->ptr);

        if(t < 10) goto finish;
  
        autofree struct string *t2 = NULL;

        i++;
        if(i < 10) goto check;

        finish:;
}

int main(int argc, char **argv)
{
// begin:;
//         struct smart_object *data = smart_object_from_json_file("res/bigo_local.json", FILE_INNER);
//         CURL *curl;
//         CURLcode res;
//
//         curl = curl_easy_init();
//         if(curl) {
//                 struct string *temp = string_alloc(0);
//                 string_cat(temp, qlkey("http://ipinfo.io/ip"));
//                 curl_easy_setopt(curl, CURLOPT_URL, temp->ptr);
//                 string_free(temp);
//
//                 /* example.com is redirected, so we tell libcurl to follow redirection */
//                 curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,"GET");
//                 curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
//                 curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, func);
//                 int num;
//                 curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)data);
//
//                 /* Perform the request, res will get the return code */
//                 res = curl_easy_perform(curl);
//                 /* Check for errors */
//                 if(res != CURLE_OK)
//                         fprintf(stderr, "curl_easy_perform() failed: %s\n",
//                 curl_easy_strerror(res));
//
//                 /* always cleanup */
//                 curl_easy_cleanup(curl);
//         }
//
//         sleep(60);
//         goto begin;
        _test(5);
        cache_free();
        dim_memory();

        return 1;
}
