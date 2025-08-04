/*
 * A little test program for md5 code which does a md5 on all the
 * bytes sent to stdin.
 */

#include <string.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>

#include "../src/md5.h"

/*
 * Print a signature
 */
static void print_sig(const unsigned char *sig) {
    const unsigned char* sig_p;

    for (sig_p = sig; sig_p < sig + MD5_SIZE; sig_p++) {
        (void)printf("%02x", *sig_p);
    }
}

/*
 * Read in from stdin and run MD5 on the input
 */
int read_file(const char* filename) {
    unsigned char sig[MD5_SIZE];
    char buffer[4096];
    md5::md5_t md5;
    bool useCin;
    std::ifstream file;

    if (filename[0] == '-') {
        useCin = true;
    } else {
        useCin = false;
        file.open(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "failed to open file " << filename << std::endl;
            exit(1);
        }
    }

    /* iterate over file */
    if (useCin) {
        const char* input = filename + 1;
        std::cout << "Processing: '" << input << "'\n";
        md5.process(input, strlen(input));
    } else {
        do {
            file.read(buffer, 4096);
            md5.process(buffer, file.gcount());
        } while (file);
    }

    md5.finish(sig);

    (void)printf("%25s '", "Resulting signature:");
    print_sig(sig);
    (void)printf("'\n");

    /* convert to string to print */
    md5::sig_to_string(sig, buffer, sizeof(buffer));
    (void)printf("%25s '%s'\n", "Results of md5_to_string:", buffer);

    /* now we convert it from string back into the sig */
    md5::sig_from_string(sig, buffer);

    (void)printf("%25s '", "After md5_from_string:");
    print_sig(sig);
    (void)printf("'\n");
}

void run_tests() {
    int ret = 0;

    unsigned char sig[MD5_SIZE], sig2[MD5_SIZE];
    char str[MD5_STRING_SIZE];

  /* MD5 Test Suite from RFC1321 */
    std::vector<std::pair<const char*, const char*> > tests;
    tests.push_back(std::pair<const char*, const char*>("", "d41d8cd98f00b204e9800998ecf8427e"));
    tests.push_back(std::pair<const char*, const char*>("a", "0cc175b9c0f1b6a831c399e269772661"));
    tests.push_back(std::pair<const char*, const char*>("abc", "900150983cd24fb0d6963f7d28e17f72"));
    tests.push_back(std::pair<const char*, const char*>("message digest", "f96b697d7cb7938d525a2f31aaf161d0"));
    tests.push_back(std::pair<const char*, const char*>("abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b"));
    tests.push_back(std::pair<const char*, const char*>("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d174ab98d277d9f5a5611c2c9f419d9f"));
    tests.push_back(std::pair<const char*, const char*>("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "57edf4a22be3c955ac49da2e2107b67a"));

    tests.push_back(std::pair<const char*, const char*>("This string is precisely 56 characters long for a reason", "93d268e9bef6608ff1a6a96adbeee106"));
    tests.push_back(std::pair<const char*, const char*>("This string is exactly 64 characters long for a very good reason", "655c37c2c8451a60306d09f2971e49ff"));
    tests.push_back(std::pair<const char*, const char*>("This string is also a specific length.  It is exactly 128 characters long for a very good reason as well. We are testing bounds.", "2ac62baa5be7fa36587c55691c026b35"));

    tests.push_back(std::pair<const char*, const char*>("aaaaaaaaaa", "7aaa7dec709fa4fa82f3746abfd80bdb"));
    tests.push_back(std::pair<const char*, const char*>("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "31e4d9c6d74cd592b78f77f72965d6ab"));
    tests.push_back(std::pair<const char*, const char*>("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "476389bd400f5643ffdcc4c202edb993"));

    std::vector<unsigned int> multiplier;
    multiplier.push_back(1);
    multiplier.push_back(1);
    multiplier.push_back(1);
    multiplier.push_back(1);
    multiplier.push_back(1);
    multiplier.push_back(1);
    multiplier.push_back(1);
    multiplier.push_back(1);
    multiplier.push_back(1);
    multiplier.push_back(1);
    multiplier.push_back(500);
    multiplier.push_back(8388608);
    multiplier.push_back(18433336);

    int passed = 0;
    int passed_h = 0;
    int passed_c = 0;

    /* run our tests */
    for (unsigned int i = 0; i < tests.size(); i++) {
        bool passed_hash = 0;
        bool passed_convert = 0;

        /* calculate the sig for our test string */
        md5::md5_t md5;

        for (unsigned int j = 0; j < multiplier[i]; j++) {
            md5.process(tests[i].first, strlen(tests[i].first));
        }

        md5.finish(sig);

        /* convert from the sig to a string rep */
        md5::sig_to_string(sig, str, sizeof(str));
        if (strcmp(str, tests[i].second) == 0) {
            passed_hash = true;
            passed_h++;
        }

        /* convert from the string back into a MD5 signature */
        md5::sig_from_string(sig2, str);
        if (memcmp(sig, sig2, MD5_SIZE) == 0) {
            passed_convert = true;
            passed_c++;
        }

        if (passed_hash and passed_convert) {
            std::cout << "TEST " << i + 1 << " PASSED" << std::endl;
            passed++;
        } else {
            std::cout << "TEST " << i + 1 << " FAILED" << std::endl;
            std::cout << "Hash: " << str << std::endl;
        }
    }

    std::cout << std::endl << "*******************************" << std::endl
              << "    " << passed << " of " << tests.size() << " tests passed" << std::endl;
    if (passed != tests.size()) {
        ret = 1;
        std::cout << std::endl << "   Please notify developer" << std::endl;
        std::cout << "  " << passed_h << " passed hashing check" << std::endl
                  << "  " << passed_h << " passed comparison check" << std::endl;
    }
    std::cout << "*******************************" << std::endl;

    exit(ret);
}

/*
 * print the usage message
 */
static void usage(void) {
    (void)fprintf(stderr, "Usage: md5_t [-r file]\n");
    exit(1);
}

int main(int argc, char **argv) {
    char do_read = 0;
    char* infile = NULL;

    argc--;
    argv++;

    /* process the args */
    for (; *argv != NULL; argc--, argv++) {
        if (**argv != '-') {
            continue;
        }

        switch (*(*argv + 1)) {
            case 'r':
                do_read = 1;
                argc--;
                argv++;

                if (argc == 0) {
                    usage();
                }

                infile = *argv;
                break;

            default:
                usage();
                break;
        }
    }

    if (argc > 0) {
        usage();
    }

    /* do we need to read in from stdin */
    if (do_read) {
        read_file(infile);
    } else {
        run_tests();
    }

    return 0;
}

