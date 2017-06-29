#include <ftw.h>
#include <cherry/stdio.h>
#include <cherry/string.h>
#include <cherry/stdlib.h>
#include <cherry/stdint.h>
#include <cherry/array.h>
#include <cherry/unistd.h>
#include <cherry/map.h>
#include <curl/curl.h>
#include <smartfox/data.h>

static int image_count = 0;

static size_t download(void *ptr, size_t size, size_t nmemb, void *d)
{
        size_t realsize = size * nmemb;
        struct file *f = file_open((char*)d, "w", FILE_INNER);
        file_write(f, ptr, realsize);
        file_close(f);

        return size * nmemb;
}

static size_t func(void *ptr, size_t size, size_t nmemb, void *d)
{
        size_t realsize = size * nmemb;

        int counter             = 0;
        struct sobj *r          = sobj_from_json(ptr, realsize, &counter);
        struct sobj *o          = sobj_get_obj(r, qlkey("output"), RPL_TYPE);
        struct string *f        = sobj_get_str(o, qlkey("url"), RPL_TYPE);
        sleep(1);
        if(f->len) {
                CURL *curl;
                CURLcode res;

                curl = curl_easy_init();
                if(curl) {
                        curl_easy_setopt(curl, CURLOPT_URL, f->ptr);

                        /* example.com is redirected, so we tell libcurl to follow redirection */
                        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download);

                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, d);

                        /* Perform the request, res will get the return code */
                        res = curl_easy_perform(curl);
                        /* Check for errors */
                        if(res != CURLE_OK)
                                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));

                        /* always cleanup */
                        curl_easy_cleanup(curl);
                }
                debug("\n");
        } else {
                debug("failed %s\n\n", (char*)d);
        }

        sobj_free(r);
        return size * nmemb;
}

static int compress(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        if(tflag == FTW_F) {
                struct string *name = string_alloc_chars((char*)fpath, strlen(fpath));
                if(string_contain(name, ".png")
                        || string_contain(name, ".jpg")
                        || string_contain(name, ".jpeg")
                        ) {
                        CURL *curl;
                        CURLcode res;

                        struct string *content = file_read_string((char*)fpath, FILE_INNER);

                        curl = curl_easy_init();
                        if(curl) {
                                curl_easy_setopt(curl, CURLOPT_URL, "https://api.tinify.com/shrink");
                                curl_easy_setopt(curl, CURLOPT_USERNAME, "api:E1EvYM4IaTrBESs_h_pF118FVyWCPxhy");

                                /* example.com is redirected, so we tell libcurl to follow redirection */
                                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,"POST");
                                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, func);

                                debug("compress : %s\n", fpath);
                                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)fpath);
                                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, content->ptr);
                                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, content->len);

                                /* Perform the request, res will get the return code */
                                res = curl_easy_perform(curl);
                                /* Check for errors */
                                if(res != CURLE_OK)
                                        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                                curl_easy_strerror(res));

                                /* always cleanup */
                                curl_easy_cleanup(curl);
                        }

                        string_free(content);
                        image_count++;
                }
                string_free(name);
        }
        return 0;
}

int main(int argc, char **argv)
{
        int flags = 0;

        if (nftw("res", compress, 20, flags) == -1) {
                perror("nftw");
                exit(EXIT_FAILURE);
        }

        debug("image count : %d\n", image_count);

        exit(EXIT_SUCCESS);
}
