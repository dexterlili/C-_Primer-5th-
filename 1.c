#include "path_handler.h"

// 解析路径, 去除路径中的 . ..
bool ResolvePath(const char *path, char *resolved_path) {
    char temp[PATH_MAX];
    char *token;
    char *ptr = temp;
  //  ./xxx/..  /   ./xx/xx/ /xx/xx
    snprintf(temp, sizeof(temp), "%s", path);

    while ((token = strtok(ptr, "/")) != NULL) {
        if (strcmp(token, ".") == 0) {
            // 当前目录，跳过
        } else if (strcmp(token, "..") == 0) {
            char *last_slash = strrchr(resolved_path, '/');
            if (last_slash != NULL) {
                // 判断resolved path 是否存在数据库中
                if(isExistDIR(resolved_path)) {
                    *last_slash = '\0'; // 回退到上一个目录
                }else{
                    puts("error path");
                    return false;
                }
            }else {
                puts("error path");
                return false;
            }
        } else {
            strcat(resolved_path, "/");
            strcat(resolved_path, token);
        }
        ptr = NULL; 
    }
    if(strlen(resolved_path) == 0) {
        strcpy(resolved_path, "/");
    }
    return true;
}


/**
 * @brief 根据当前的绝对路径和目标的相对路径，获取目标的绝对路径
 * @param usr_path 用户的根目录绝对路径
 * @param curr_path 用户所在的相对路径
 * @param rlt_path 想要转换格式的相对路径
 * @param abs_path 传入传出参数，传入用于接收目标绝对路径的字符数组，传出目标绝对路径字符串
 * @return int 成功返回0 失败返回-1
 */
int getRealPath(const char *usr_path, const char *curr_path, const char *rlt_path, char *abs_path)
{
    char path[128] = {};
    // 判断相对路径的首位字符是什么
    char ch = rlt_path[0];
    switch (ch)
    {
    case '/':
        sprintf(path, "%s%s%s", usr_path, "/", rlt_path); // 获取未化简的目标绝对路径
        break;
    case ' ':
        return -1; // 路径输入错误
    default:
        if (curr_path[strlen(curr_path) - 1] == '/')
        {
            sprintf(path, "%s%s%s", usr_path, curr_path, rlt_path); // 获取未化简的目标绝对路径
        }
        else
        {
            sprintf(path, "%s%s%s%s", usr_path, curr_path, "/", rlt_path); // 获取未化简的目标绝对路径
        }

        break;
    }
    char *temp = realpath(path, NULL);
    if (temp == NULL)
    {
        // TODO:错误处理
        return -1;
    }
    // 拷贝目标绝对路径
    strcpy(abs_path, temp);
    // 释放realpath分配的内存
    free(temp);
    return 0;
}

/**
 * @brief 判断输入绝对路径是否为合法目录
 * @param usr_path 用户的根目录绝对路径
 * @param abs_path 想要判断的目录绝对路径
 * @return int 返回-1表示路径不存在，返回-2表示不是目录是个文件，-3表示路径不合法
 */
int isLegalPath(const char *usr_path, const char *abs_path)
{
    // 判断输入路径是否为目录
    struct stat statbuf;
    int res = -1;
    res = lstat(abs_path, &statbuf); // 获取linux操作系统下文件的属性
    if (0 != res)
    {
        // TODO:错误处理,路径输入错误
        return -1;
    }
    if (!S_ISDIR(statbuf.st_mode))
    {
        return -2;
    }
    // 获取用户的根目录绝对路径长度len，判断前len个字符是否相等
    int len = strlen(usr_path);
    int ret = strncmp(usr_path, abs_path, len);
    if (ret != 0)
    {
        return -3;
    }
    return 0;
}

/**
 * @brief 判断输入绝对路径是否为合法文件
 * @param usr_path 用户的根目录绝对路径
 * @param abs_path 想要判断的文件绝对路径
 * @return bool 合法文件返回true 非法文件返回false
 */
bool isLegalFile(const char *usr_path, const char *abs_path)
{
    // 判断输入路径是否为文件
    struct stat statbuf;
    int res = -1;
    res = lstat(abs_path, &statbuf); // 获取linux操作系统下文件的属性
    if (0 != res)
    {
        // TODO:错误处理,路径输入错误
        return false;
    }
    if (!S_ISREG(statbuf.st_mode))
    {
        return false;
    }
    // 获取用户的根目录绝对路径长度len，判断前len个字符是否相等
    int len = strlen(usr_path);
    int ret = strncmp(usr_path, abs_path, len);
    if (ret != 0)
    {
        return false;
    }
    return true;
}

/**
 * @brief 获取完整文件路径名
 * @param file_path 传入传出参数，用户上传的完整文件路径名
 * @param cmd_share_var 客户端的用户信息
 * @param path_name 用户当前所在相对路径 空格 用户要上传的文件
 * @return int 成功返回0 失败返回-1
 */
int getUpFilePath(char *file_path, CMDShareVar *cmd_share_var, const char *path_name)
{
    char path[128] = {};
    strcpy(path, path_name);
    int path_len = strlen(path);
    if (path[path_len - 1] == '\n')
    {
        path[path_len - 1] = '\0';
    }
    // 第一个参数，当前相对路径，第二个参数，文件名
    const char *delim = " ";
    char *saveptr;
    // 分割指令
    // 服务器将curr_path获取修改成第一行的指令，并注释第二行
    // 调试时，注释第一行，恢复第二行
    char *curr_path = strtok_r(path, delim, &saveptr); // 获取用户的相对路径
    // char *curr_path = "/";
    char *name = strtok_r(NULL, delim, &saveptr); // 获取文件名
    // 获取文件绝对路径
    char usr_path[200] = {};
    char real_usr_path[200] = {};

    sprintf(usr_path, "%s/%s", cmd_share_var->base_path, cmd_share_var->user_name);
    getRealPath(usr_path, "/", "/", real_usr_path);
    // 拼接完整文件名
    sprintf(file_path, "%s%s/%s", real_usr_path, curr_path, name);

    return 0;
}

/**
 * @brief 获取完整文件路径名
 * @param real_usr_path 传入传出参数，用户根目录绝对路径
 * @param file_path 传入传出参数，用户下载的完整文件路径名
 * @param cmd_share_var 客户端的用户信息
 * @param path_name 用户当前所在相对路径 空格 用户要下载的文件
 * @return int 成功返回0 失败返回-1
 */
int getDOWNFilePath(char *real_usr_path, char *file_path, CMDShareVar *cmd_share_var, const char *path_name)
{
    // 拷贝参数并去除可能会存在的换行符
    char path[128] = {};
    strcpy(path, path_name);
    int path_len = strlen(path);
    if (path[path_len - 1] == '\n')
    {
        path[path_len - 1] = '\0';
    }

    // 第一个参数，当前相对路径，第二个参数，文件名
    const char *delim = " ";
    char *saveptr;
    // 分割指令
    // 服务器运行时将curr_path获取修改成第一行的指令，并注释第二行
    // 调试时注释第一行，恢复第二行
    char *curr_path = strtok_r(path, delim, &saveptr); // 获取用户的相对路径
    // char *curr_path = "/";                        // 暂时把相对路径设置成"/"
    char *name = strtok_r(NULL, delim, &saveptr); // 获取文件名
    // 验证文件是否存在
    //  获取文件绝对路径
    char usr_path[200] = {}; // 用户根目录相对路径

    sprintf(usr_path, "%s/%s", cmd_share_var->base_path, cmd_share_var->user_name);
    int ret = getRealPath(usr_path, "/", "/", real_usr_path);
    if (ret == -1)
    {
        ERROR_CHECK(ret, -1, "getRealPath");
        return -1;
    }
    ret = getRealPath(real_usr_path, curr_path, name, file_path); // 根据文件路径拼接完整的文件名
    if (ret == -1)
    {
        ERROR_CHECK(ret, -1, "getRealPath");
        return -1;
    }
    return 0;
}