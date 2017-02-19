#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

char *user = NULL, *target=NULL;
int mtime = -1;
bool dont_cross_vol = false;

void find(char *path){
    DIR *dirp;
    struct dirent *dp;

    if ((dirp = opendir(path)) == NULL) {
        perror("Failed to open file.");
        return;
    }

    char resolved_path[PATH_MAX+1];
    do {
        if ((dp = readdir(dirp) != NULL){
            struct stat st;
            if (fstat(dp->ino, &st)) {
                perror("Failed to stat file");
                return;
            }

            realpath(dp->name, resolved_path);
            struct *pw = getpwuid(st.st_uid);
            struct *grp = getgrgid(st.st_gid);
            printf("%04x/%d\t%s\t%d\t%s\t%s\t%d\t%s\t%s\n", st.st_dev, st.st_ino, st.st_mode,
                        st.st_nlink, pw->pw_name, grp->gr_name,
                        st.st_size, asctime(localtime(st.st_mtime)), resolved_path);
            if(S_ISDIR(st.st_mode)) find(resolved_path);
        }
    } while (dp != NULL);

    closedir(dirp);
}

int main() {


    char starting_dir[PATH_MAX+1];

    while ((c = getopt(argc, argv, "u:m:xl:")) != -1)
        switch (c) {
            //user only list nodes which are owned by the specified user
            case 'u':
                user = optarg;
                break;
            //mtime -- only list nodes which have not been modified in at least
            //that many seconds
            case 'm':
                int tmp = atoi(optarg);
                if(tmp < 0) fprintf(stderr, "Usage Error: mtime must be a positive integer\n");
                mtime = tmp;
                break;
            //dont cross volume boundaries
            case 'x':
                dont_cross_vol = true;
                break;
            //only print info about nodes which are symlinks whose targets
            //successfully resolve to another node called target
            case 'l':
                target = optarg;
                break;
            case '?':
                if (optopt == 'u' || optopt == 'm' || optopt == 'x' || optopt == 'l') {
                    fprintf(stderr, "Usage Error: Option -%c requires an argument.\n", optopt);
                }
                return -1;
                break;
            default:
                fprintf(stderr, "Usage: [-u user] [-m mtime] [-x] [-l target]");
                return -1;
        }

        //use the current directory if none is specified
        if ( optind < argc) starting_dir = argv[optind];
        else getwd(starting_dir);
}
