// #define _XOPEN_SOURCE 500
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
#include <xxtea.h>

#define head_key "0928120358194734123490234902384023412398471928374"
#define xml_key "PiPoGame2016"

static struct sobj *config = NULL;
static struct sobj *keystore_config = NULL;
static struct string *old_package = NULL;

static int apk_counts = 0;
static int keystore_counts = 0;

static int __valid_apk(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        if(tflag == FTW_F) {
                struct string *name = string_alloc_chars((char*)fpath, strlen(fpath));
                if(string_contain(name, ".apk")) {
                        apk_counts++;
                }
                string_free(name);
        }

        return 0;
}

static int __valid_keystore(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        if(tflag == FTW_F) {
                struct string *name = string_alloc_chars((char*)fpath, strlen(fpath));
                if(string_contain(name, ".keystore")) {
                        keystore_counts++;
                }
                string_free(name);
        }

        return 0;
}

static void __extract_apk(char *file, size_t len)
{
        struct string *command = string_alloc(0);
        string_cat(command, qlkey("apktool.bat d "));
        string_cat(command, file, len);
        string_cat(command, qlkey(" -f -o output/temp\n"));
        system(command->ptr);
        string_free(command);
}

static int __encrypt_assets(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        if(tflag == FTW_F) {
                struct string *name = string_alloc_chars((char*)fpath, strlen(fpath));
                if(string_contain(name, ".png")
                                || string_contain(name, ".jpg")
                                || string_contain(name, ".jpeg")) {
                        struct string *content = file_read_string((char*)fpath, FILE_INNER);
                        if(memcmp(content->ptr, head_key, sizeof(head_key) - 1) != 0) {
                                struct string *new_content = string_alloc_chars(qlkey(head_key));
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
                        if(memcmp(content->ptr, head_key, sizeof(head_key) - 1) != 0) {
                                xxtea_long ret_length = 0;
                                unsigned char* en = xxtea_encrypt(content->ptr, content->len, (unsigned char*)xml_key, sizeof(xml_key)-1, &ret_length);

                                struct string *new_content = string_alloc_chars(qlkey(head_key));
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

static void __replace_link_config()
{
        struct sobj *object_config = sobj_get_obj(config, qlkey("config"), RPL_TYPE);
        struct string *content = sobj_to_json(object_config);

        debug("replace game config success \n%s\n\n", content->ptr);

        xxtea_long ret_length = 0;
        unsigned char* en = xxtea_encrypt(content->ptr, content->len, (unsigned char*)xml_key, sizeof(xml_key)-1, &ret_length);

        struct string *new_content = string_alloc_chars(qlkey(head_key));
        string_cat(new_content, en, ret_length);

        free(en);

        struct file *f = file_open("output/temp/assets/res/link_config.json", "w", FILE_INNER);
        file_write(f, new_content->ptr, new_content->len);
        file_close(f);
        string_free(new_content);
        string_free(content);
}

static void __replace_strings()
{
        struct xml_element *root = xml_parse("output/temp/res/values/strings.xml", FILE_INNER);

        struct sobj *strings = sobj_get_obj(config, qlkey("strings"), RPL_TYPE);

        struct array *strings_key = array_alloc(sizeof(struct bytes *), ORDERED);
        map_get_list_key(strings->data, strings_key);
        struct bytes **key;
        array_for_each(key, strings_key) {
                struct string *k = string_alloc_chars((*key)->ptr, (*key)->len);
                struct string *val = sobj_get_str(strings, (*key)->ptr, (*key)->len, RPL_TYPE);

                struct xml_element *e = xml_find_deep(root, "string", "name", k->ptr);
                if(e) {
                        debug("replace success key:'%s' fromValue:'%s' toValue:'%s' ...\n",
                                k->ptr, e->value->ptr, val->ptr);

                        e->value->len = 0;
                        string_cat_string(e->value, val);
                }

                string_free(k);
        }
        array_free(strings_key);

        struct string *result = xml_to_string(root);
        struct file *f = file_open("output/temp/res/values/strings.xml", "w", FILE_INNER);
        file_write(f, result->ptr, result->len);
        file_close(f);
        xml_free(root);
}

static void __fix_package(const char *path)
{
        struct string *content = file_read_string((char *)path, FILE_INNER);

        struct string *old_slashpackage = string_alloc_chars(old_package->ptr, old_package->len);
        string_replace(old_slashpackage, ".", "/");

        struct string *package = sobj_get_str(config, qlkey("package"), RPL_TYPE);
        struct string *slashpackage = string_alloc(0);
        string_cat_string(slashpackage, package);
        string_replace(slashpackage, ".", "/");

        if(package->len == 0) {
                debug("ERROR : please provide new package name!\n");
                exit(EXIT_FAILURE);
        }

        if(old_package->len == 0) {
                debug("ERROR : apk corrupt because old package is empty!\n");
                exit(EXIT_FAILURE);
        }

        if(strcmp(old_package->ptr, package->ptr) == 0) return;

        string_replace(content, old_package->ptr, package->ptr);
        string_replace(content, old_slashpackage->ptr, slashpackage->ptr);

        string_free(slashpackage);
        string_free(old_slashpackage);

        struct file *f = file_open((char *)path, "w", FILE_INNER);
        file_write(f, content->ptr, content->len);
        file_close(f);
        string_free(content);
}

static int fix_package(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        if(tflag == FTW_F) {
                __fix_package(fpath);
        }

        return 0;
}

static int __do_replace_file(char *from, char *to)
{
        struct string *content = file_read_string(from, FILE_INNER);
        if(content) {
                struct file *f = file_open((char *)to, "w", FILE_INNER);
                file_write(f, content->ptr, content->len);
                file_close(f);
                string_free(content);
                return 1;
        }
        return 0;
}

static int __replace_icon(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        if(tflag == FTW_D) {
                struct string *path = string_alloc_chars((char *)fpath, strlen(fpath));
                string_cat(path, qlkey("/icon.png"));
                int r = 0;
                if(string_contain(path, "drawable-hdpi")) {
                        r = __do_replace_file("res/icon/drawable-hdpi/icon.png", path->ptr);

                } else if(string_contain(path, "drawable-ldpi")) {
                        r = __do_replace_file("res/icon/drawable-ldpi/icon.png", path->ptr);

                } else if(string_contain(path, "drawable-mdpi")) {
                        r = __do_replace_file("res/icon/drawable-mdpi/icon.png", path->ptr);

                } else if(string_contain(path, "drawable-xhdpi")) {
                        r = __do_replace_file("res/icon/drawable-xhdpi/icon.png", path->ptr);

                } else if(string_contain(path, "drawable-xxhdpi")) {
                        r = __do_replace_file("res/icon/drawable-xxhdpi/icon.png", path->ptr);
                }

                if(r == 1) {
                        debug("replace success : %s ...\n", path->ptr);
                }

                string_free(path);
        }
        return 0;
}

static void __replace_logo()
{
        int r = __do_replace_file("res/logo/logo_game.png", "output/temp/assets/res-HD/texture/scene/logo_game.png");
        if(r == 1) {
                debug("replace success : %s ...\n", "output/temp/assets/res-HD/texture/scene/logo_game.png");
        }
}

static void __fix_version_code()
{
        struct xml_element *root = xml_parse("output/temp/AndroidManifest.xml", FILE_INNER);
        struct xml_element *manifest = xml_find(root, "manifest", 0);

        struct string *android_version_code = sobj_get_str(config, qlkey("android_version_code"), RPL_TYPE);
        string_trim(android_version_code);
        if(android_version_code->len) {
                xml_element_set_attribute(manifest, qlkey("android:versionCode"), qskey(android_version_code));
                debug("replace version code : %s ...\n", android_version_code->ptr);
        }

        struct string *android_version_name = sobj_get_str(config, qlkey("android_version_name"), RPL_TYPE);
        string_trim(android_version_name);
        if(android_version_name->len) {
                xml_element_set_attribute(manifest, qlkey("android:versionName"), qskey(android_version_name));
                debug("replace version name : %s ...\n", android_version_name->ptr);
        }

        struct string *result = xml_to_string(root);
        struct file *f = file_open("output/temp/AndroidManifest.xml", "w", FILE_INNER);
        file_write(f, result->ptr, result->len);
        file_close(f);
        xml_free(root);
}

static int decode(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        if(tflag == FTW_F) {
                struct string *name = string_alloc_chars((char*)fpath, strlen(fpath));
                if(string_contain(name, ".apk")) {

                        {
                                FILE *fp;
                                char path[1035];

                                /* Open the command for reading. */
                                fp = popen("mkdir -p output", "r");
                                if (fp == NULL) {
                                        printf("Failed to run command\n" );
                                        exit(1);
                                }

                                /* Read the output a line at a time - output it. */
                                while (fgets(path, sizeof(path)-1, fp) != NULL) {
                                        debug("%s\n", path);
                                }

                                /* close */
                                pclose(fp);
                        }
                        struct string *command = string_alloc(0);
                        string_cat(command, qlkey("apktool.bat d "));
                        string_cat_string(command, name);
                        string_cat(command, qlkey(" -f -o output/temp\n"));
                        debug("S: %s\n", command->ptr);
                        system(command->ptr);
                        string_free(command);
                }
                string_free(name);
        }

        return 0;
}

static int __resign(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        if(tflag == FTW_F) {
                struct string *name = string_alloc_chars((char*)fpath, strlen(fpath));
                if(string_contain(name, ".keystore")) {

                        struct string *alias = sobj_get_str(keystore_config, qlkey("alias"), RPL_TYPE);
                        string_trim(alias);
                        if(alias->len == 0) {
                                debug("\nE: please provide keystore alias\n");
                                exit(EXIT_FAILURE);
                        }

                        struct string *pass = sobj_get_str(keystore_config, qlkey("pass"), RPL_TYPE);
                        string_trim(pass);
                        if(pass->len == 0) {
                                debug("\nE: please provide keystore password\n");
                                exit(EXIT_FAILURE);
                        }

                        struct string *command = string_alloc(0);
                        string_cat(command, qlkey("jarsigner -verbose -sigalg MD5withRSA -digestalg SHA1 -keystore "));
                        string_cat_string(command, name);
                        string_cat(command, qlkey(" -storepass "));
                        string_cat_string(command, pass);
                        string_cat(command, qlkey(" -signedjar output/unaligned_app.apk output/rebuild_app.apk "));
                        string_cat_string(command, alias);
                        debug("S: %s\n", command->ptr);
                        system(command->ptr);
                        string_free(command);
                }
        finish:;
                string_free(name);
        }
        return 0;
}

static int __print_keyhash(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
        if(tflag == FTW_F) {
                struct string *name = string_alloc_chars((char*)fpath, strlen(fpath));
                if(string_contain(name, ".keystore")) {

                        struct string *alias = sobj_get_str(keystore_config, qlkey("alias"), RPL_TYPE);
                        string_trim(alias);
                        if(alias->len == 0) {
                                debug("\nE: please provide keystore alias\n");
                                exit(EXIT_FAILURE);
                        }

                        struct string *pass = sobj_get_str(keystore_config, qlkey("pass"), RPL_TYPE);
                        string_trim(pass);
                        if(pass->len == 0) {
                                debug("\nE: please provide keystore password\n");
                                exit(EXIT_FAILURE);
                        }

                        struct string *command = string_alloc(0);
                        string_cat(command, qlkey("keytool -export -alias "));
                        string_cat_string(command, alias);
                        string_cat(command, qlkey(" -storepass "));
                        string_cat_string(command, pass);
                        string_cat(command, qlkey(" -keystore "));
                        string_cat_string(command, name);
                        string_cat(command, qlkey(" | openssl sha1 -binary | openssl base64"));
                        // debug("S: %s\n", command->ptr);
                        debug("\nprint keyhash : ");
                        debug(PRINT_YEL "\n" PRINT_RESET);
                        system(command->ptr);
                        string_free(command);
                }
        finish:;
                string_free(name);
        }
        return 0;
}

static void __zip_align()
{
        struct string *command = string_alloc(0);
        string_cat(command, qlkey("zipalign -f 4 output/unaligned_app.apk output/release_app.apk"));
        system(command->ptr);
        string_free(command);
}

static int compress()
{
        struct string *command = string_alloc(0);
        string_cat(command, qlkey("apktool.bat b"));
        string_cat(command, qlkey(" output/temp"));
        string_cat(command, qlkey(" -f -o output/rebuild_app.apk\n"));
        system(command->ptr);
        string_free(command);
}

int main(int argc, char **argv)
{
        config = sobj_from_json_file("res/config.json", FILE_INNER);
        keystore_config = sobj_from_json_file("res/keystore/info.json", FILE_INNER);
        old_package = string_alloc(0);
        int flags = 0;

        if (nftw("res/apk", __valid_apk, 20, flags) == -1) {
                perror("nftw");
                exit(EXIT_FAILURE);
        }

        if (nftw("res/keystore", __valid_keystore, 20, flags) == -1) {
                perror("nftw");
                exit(EXIT_FAILURE);
        }

        if(apk_counts == 0) {
                debug("Please provide apk file!\n");
                exit(EXIT_FAILURE);
        }

        if(apk_counts > 1) {
                debug("Only one apk can be processed a time!\n");
                exit(EXIT_FAILURE);
        }

        if(keystore_counts == 0) {
                debug("Please provide keystore file!\n");
                exit(EXIT_FAILURE);
        }

        if(keystore_counts > 1) {
                debug("Only one keystore can be processed a time!\n");
                exit(EXIT_FAILURE);
        }

        if (nftw("res/apk", decode, 20, flags) == -1) {
                perror("nftw");
                exit(EXIT_FAILURE);
        }
        /*
         * read old package
         */
        {
                struct xml_element *root = xml_parse("output/temp/AndroidManifest.xml", FILE_INNER);
                struct xml_element *manifest = xml_find(root, "manifest", 0);
                struct xml_attribute *package = xml_find_attribute(manifest, "package");
                old_package->len = 0;
                string_cat_string(old_package, package->value);
        }

        __replace_link_config();
        //
        __replace_strings();

        if (nftw("output/temp/smali", fix_package, 20, flags) == -1) {
                perror("nftw");
                exit(EXIT_FAILURE);
        }
        __fix_package("output/temp/AndroidManifest.xml");
        // __fix_package("output/temp/original/AndroidManifest.xml");
        __fix_version_code();

        if (nftw("output/temp/res", __replace_icon, 20, flags) == -1) {
                perror("nftw");
                exit(EXIT_FAILURE);
        }

        __replace_logo();

        if (nftw("output/temp/assets", __encrypt_assets, 20, flags) == -1) {
                perror("nftw");
                exit(EXIT_FAILURE);
        }

        compress();
        sleep(2);

        if (nftw("res/keystore", __resign, 20, flags) == -1) {
                perror("nftw");
                exit(EXIT_FAILURE);
        }
        sleep(2);

        __zip_align();
        sleep(2);

        system("rm -rf output/temp");
        system("rm output/rebuild_app.apk");
        system("rm output/unaligned_app.apk");

        // if (nftw("res/keystore", __print_keyhash, 20, flags) == -1) {
        //         perror("nftw");
        //         exit(EXIT_FAILURE);
        // }

        debug("\n\nSUCCESS\n");
        debug("\npress q to exit\n");

        char c;
        do {
                scanf("%c\n", &c);
                if(c == 'q') {
                        break;
                }
        } while(1);

        sobj_free(config);
        string_free(old_package);
        exit(EXIT_SUCCESS);
}
