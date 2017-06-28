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

static int process_count = 0;

static int encrypt(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        debug("%s\n",fpath);
        if(tflag == FTW_F) {
                char *path = fpath + 4;
                struct string *name = string_alloc_chars((char*)path, strlen(path));
                struct string *content = file_read_string((char*)fpath, FILE_INNER);

                if(memcmp(content->ptr, key, sizeof(key) - 1) == 0) {
                        string_free(content);
                        string_free(name);
                        return 0;
                }

                struct string *new_content      = string_alloc_chars(qskey(name));
                string_cat(new_content, qlkey(key));
                string_cat_string(new_content, content);
                string_free(content);
                content                         = new_content;

                xxtea_long ret_length           = 0;
                unsigned char* en = xxtea_encrypt(content->ptr, content->len, (unsigned char*)xml_key, sizeof(xml_key)-1, &ret_length);
                new_content                     = string_alloc_chars(qlkey(key));
                string_cat(new_content, en, ret_length);
                free(en);
                string_free(content);
                content                         = new_content;

                struct string *path_to_write = string_alloc_chars(qlkey("output/data"));
                string_cat_int(path_to_write, process_count);
                struct file *f = file_open(path_to_write->ptr, "w", FILE_INNER);
                file_write(f, content->ptr, content->len);
                file_close(f);

                string_free(content);
                string_free(name);
                string_free(path_to_write);

                process_count++;
                debug("write %d\n", process_count);
        }

        return 0;
}

static int decrypt(char *path) {

        struct string *name = string_alloc(0);

check:;
        name->len = 0;
        string_cat(name, qlkey("output/data"));
        string_cat_int(name, process_count);

        struct file *f = file_open(name->ptr, "r", FILE_INNER);
        if(!f->ptr) {
                file_close(f);
                goto finish;
        }

        

        file_close(f);
        process_count++;
        goto check;

finish:;
        string_free(name);
}

// static int decrypt(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
// {
//         debug("%s\n", name->ptr);
//         if(tflag == FTW_F) {
//                 char *path = fpath + 4;
//                 struct string *name = string_alloc_chars((char*)path, strlen(path));
//
//                 // if(string_contain(name, ".png")
//                 //                 || string_contain(name, ".jpg")
//                 //                 || string_contain(name, ".jpeg")) {
//                 //         struct string *content = file_read_string((char*)fpath, FILE_INNER);
//                 //         if(memcmp(content->ptr, key, sizeof(key) - 1) == 0) {
//                 //                 struct string *new_content = string_alloc(0);
//                 //                 string_cat_string(new_content, content);
//                 //
//                 //                 int l = sizeof(key) - 1;
//                 //
//                 //                 struct file *f = file_open((char*)fpath, "w", FILE_INNER);
//                 //                 file_write(f, new_content->ptr + l, new_content->len - l);
//                 //                 file_close(f);
//                 //                 string_free(new_content);
//                 //         }
//                 //         string_free(content);
//                 // } else if(string_contain(name, ".xml")) {
//                 //         struct string *content = file_read_string((char*)fpath, FILE_INNER);
//                 //         if(memcmp(content->ptr, key, sizeof(key) - 1) == 0) {
//                 //
//                 //                 int l = sizeof(key) - 1;
//                 //
//                 //                xxtea_long enc_length = 0;
//                 //                unsigned char* rr = xxtea_decrypt(content->ptr + l, content->len - l, (unsigned char*)xml_key, sizeof(xml_key)-1, &enc_length);
//                 //
//                 //                 struct file *f = file_open((char*)fpath, "w", FILE_INNER);
//                 //                 file_write(f, rr, enc_length);
//                 //                   free(rr);
//                 //                 file_close(f);
//                 //         }
//                 //         string_free(content);
//                 // }
//                 string_free(name);
//         }
//
//         return 0;
// }

int main(int argc, char *argv[])
{
        int flags = 0;

        if(strcmp(argv[2], "encrypt") == 0) {
                if (nftw(argv[1], encrypt, 20, flags) == -1) {
                        perror("nftw");
                        exit(EXIT_FAILURE);
                }
        } else if(strcmp(argv[2], "decrypt") == 0) {
                decrypt(argv[1]);
                // if (nftw(argv[1], decrypt, 20, flags) == -1) {
                //         perror("nftw");
                //         exit(EXIT_FAILURE);
                // }
        }

        exit(EXIT_SUCCESS);
}
