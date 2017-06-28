#define _XOPEN_SOURCE 500
#include <ftw.h>
#include <cherry/stdio.h>
#include <cherry/string.h>
#include <cherry/stdlib.h>
#include <cherry/stdint.h>
#include <cherry/array.h>
#include <cherry/map.h>
#include <xxtea.h>

#define key "0928120358194734123490234902384023412398471928374"
#define xml_key "PiPoGame2016"

static int encrypt(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        if(tflag == FTW_F) {
                struct string *name = string_alloc_chars((char*)fpath, strlen(fpath));
                if(string_contain(name, ".png")
                                || string_contain(name, ".jpg")
                                || string_contain(name, ".jpeg")) {
                        struct string *content = file_read_string((char*)fpath, FILE_INNER);
                        if(memcmp(content->ptr, key, sizeof(key) - 1) != 0) {
                                struct string *new_content = string_alloc_chars(qlkey(key));
                                // struct string *new_content = string_alloc(0);
                                string_cat_string(new_content, content);

                                // int l = sizeof(key) - 1;
                                int l = 0;

                                struct file *f = file_open((char*)fpath, "w", FILE_INNER);
                                file_write(f, new_content->ptr + l, new_content->len - l);
                                file_close(f);
                                string_free(new_content);
                        }
                        string_free(content);
                } else if(string_contain(name, ".xml")) {
                        struct string *content = file_read_string((char*)fpath, FILE_INNER);
                        if(memcmp(content->ptr, key, sizeof(key) - 1) != 0) {
                                xxtea_long ret_length = 0;
                                unsigned char* en = xxtea_encrypt(content->ptr, content->len, (unsigned char*)xml_key, sizeof(xml_key)-1, &ret_length);

                                struct string *new_content = string_alloc_chars(qlkey(key));
                                string_cat(new_content, en, ret_length);

                                free(en);

                                // int l = sizeof(key) - 1;
                                int l = 0;

                                struct file *f = file_open((char*)fpath, "w", FILE_INNER);
                                file_write(f, new_content->ptr + l, new_content->len - l);
                                file_close(f);
                                string_free(new_content);
                        }
                        string_free(content);
                }
                string_free(name);
        }

        return 0;
}

static int decrypt(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        if(tflag == FTW_F) {
                struct string *name = string_alloc_chars((char*)fpath, strlen(fpath));
                if(string_contain(name, ".png")
                                || string_contain(name, ".jpg")
                                || string_contain(name, ".jpeg")) {
                        struct string *content = file_read_string((char*)fpath, FILE_INNER);
                        if(memcmp(content->ptr, key, sizeof(key) - 1) == 0) {
                                struct string *new_content = string_alloc(0);
                                string_cat_string(new_content, content);

                                int l = sizeof(key) - 1;

                                struct file *f = file_open((char*)fpath, "w", FILE_INNER);
                                file_write(f, new_content->ptr + l, new_content->len - l);
                                file_close(f);
                                string_free(new_content);
                        }
                        string_free(content);
                } else if(string_contain(name, ".xml")) {
                        struct string *content = file_read_string((char*)fpath, FILE_INNER);
                        if(memcmp(content->ptr, key, sizeof(key) - 1) == 0) {

                                int l = sizeof(key) - 1;

                               xxtea_long enc_length = 0;
                               unsigned char* rr = xxtea_decrypt(content->ptr + l, content->len - l, (unsigned char*)xml_key, sizeof(xml_key)-1, &enc_length);

                                struct file *f = file_open((char*)fpath, "w", FILE_INNER);
                                file_write(f, rr, enc_length);
                                  free(rr);
                                file_close(f);
                        }
                        string_free(content);
                }
                string_free(name);
        }

        return 0;
}

int main(int argc, char *argv[])
{
        int flags = 0;

        if(strcmp(argv[2], "encrypt") == 0) {
                if (nftw(argv[1], encrypt, 20, flags) == -1) {
                        perror("nftw");
                        exit(EXIT_FAILURE);
                }
        } else if(strcmp(argv[2], "decrypt") == 0) {
                if (nftw(argv[1], decrypt, 20, flags) == -1) {
                        perror("nftw");
                        exit(EXIT_FAILURE);
                }
        }



        exit(EXIT_SUCCESS);
}
