// cp path1 path2
int select_sql_file(CMDShareVar *cmd_share_var, char *select_sql, MYSQL_RES **res)
{
    int ret = mysql_query(cmd_share_var->mysql, select_sql);
    MYSQL_ERROR_CHECK(cmd_share_var->mysql, !ret, 0, "mysql_query");
    *res = mysql_store_result(cmd_share_var->mysql);
    // row = mysql_fetch_row(res);
    return 0;
}
int insert_sql_file(CMDShareVar *cmd_share_var, char *insert_sql)
{
    int ret = mysql_query(cmd_share_var->mysql, insert_sql);
    MYSQL_ERROR_CHECK(cmd_share_var->mysql, !ret, 0, "mysql_query");
    // row = mysql_fetch_row(res);
    return 0;
}
int update_sql_file(CMDShareVar *cmd_share_var, char *update_sql)
{
    int ret = mysql_query(cmd_share_var->mysql, update_sql);
    MYSQL_ERROR_CHECK(cmd_share_var->mysql, !ret, 0, "mysql_query");
    // row = mysql_fetch_row(res);
    return 0;
}

// cp文件到文件，存在则覆盖，不存在就新建
int cmd_cp_file(CMDShareVar *cmd_share_var, char *src, char *dest)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char select_sql[300] = {};
    char insert_sql[300] = {};
    char update_sql[300] = {};

    // 判断传入的dest参数是否存在，不存在就新建，存在就覆盖
    sprintf(select_sql, "select id, user_id from file_info where user_id = %d and file_path like '%%%s' and file_type = 1", cmd_share_var->user_id, dest);
    select_sql_file(cmd_share_var, select_sql, &res);
    row = mysql_fetch_row(res);
    if (res == NULL || mysql_num_rows(res) == 0) // dest不存在，新建一个文件
    {
        bzero(select_sql, sizeof(select_sql));
        sprintf(select_sql, "select user_id, parent_id, hash_sha, hash_md5 from file_info where user_id = %d and file_path like '%%%s'", cmd_share_var->user_id, src);
        select_sql_file(cmd_share_var, select_sql, &res);
        row = mysql_fetch_row(res);
        bzero(insert_sql, sizeof(insert_sql));
        // TODO: file_path未写
        sprintf(insert_sql, "insert into file_info (file_name, user_id, parent_id, file_path, file_type, hash_sha, hash_md5) values \
                                        ('%s', %d, %d, '%s', 1, '%s', '%s')",
                dest, atoi(row[0]), atoi(row[1]), TODO, row[2], row[3]);
        insert_sql_file(cmd_share_var, insert_sql);
    }
    else // dest存在，覆盖原文件
    {
        bzero(select_sql, sizeof(select_sql));
        sprintf(select_sql, "select hash_sha, hash_md5 from file_info where user_id = %d and file_path = '%s'", cmd_share_var->user_id, src);
        select_sql_file(cmd_share_var, select_sql, &res);
        row = mysql_fetch_row(res);
        bzero(update_sql, sizeof(update_sql));
        sprintf(update_sql, "update file_info set hash_sha = '%s', hash_md5 = '%s' where user_id = %d and file_path = '%s'", row[0], row[1], cmd_share_var->user_id, dest);
        update_sql_file(cmd_share_var, update_sql);
    }
    mysql_free_result(res);
    return 0;
}

int cmd_cp_dir(CMDShareVar *cmd_share_var, char *src, char *dest)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char select_sql[300] = {};
    char insert_sql[300] = {};
    char update_sql[300] = {};
    int parent_id;
    int id;

    // 判断传入的dest参数目录下是否有同名文件存在，不存在就新建，存在就覆盖
    sprintf(select_sql, "select id from file_info where user_id = %d and file_path like '%%%s'", cmd_share_var->user_id, dest);
    select_sql_file(cmd_share_var, select_sql, &res);
    row = mysql_fetch_row(res);
    parent_id = atoi(row[0]);

    sprintf(select_sql, "select id from file_info where parent_id = %d and file_type = 1", parent_id);
    select_sql_file(cmd_share_var, select_sql, &res);
    row = mysql_fetch_row(res);
    if (res == NULL || mysql_num_rows(res) == 0) // dest下不存在和src的同名文件不存在，新建一个文件
    {
        bzero(select_sql, sizeof(select_sql));
        sprintf(select_sql, "select user_id, parent_id, hash_sha, hash_md5 from file_info where user_id = %d and file_path like '%%%s'", cmd_share_var->user_id, src);
        select_sql_file(cmd_share_var, select_sql, &res);
        row = mysql_fetch_row(res);
        bzero(insert_sql, sizeof(insert_sql));
        // TODO: file_path未写
        sprintf(insert_sql, "insert into file_info (file_name, user_id, parent_id, file_path, file_type, hash_sha, hash_md5) values \
                                        ('%s', %d, %d, '%s', 1, '%s', '%s')",
                src, atoi(row[0]), parent_id, TODO, row[2], row[3]);
        insert_sql_file(cmd_share_var, insert_sql);
    }
    else // dest下存在一个同名文件，覆盖原文件
    {
        id = atoi(row[0]);
        bzero(select_sql, sizeof(select_sql));
        sprintf(select_sql, "select hash_sha, hash_md5 from file_info where user_id = %d and file_path = '%s'", cmd_share_var->user_id, src);
        select_sql_file(cmd_share_var, select_sql, &res);
        row = mysql_fetch_row(res);
        bzero(update_sql, sizeof(update_sql));
        sprintf(update_sql, "update file_info set hash_sha = '%s', hash_md5 = '%s' where id = %d ", row[0], row[1], id);
        update_sql_file(cmd_share_var, update_sql);
    }
    mysql_free_result(res);
    return 0;
}

// cp文件到目录下，目录必须存在

/**
 * @brief cp命令函数丐版，仅仅支持单文件或多文件复制，不支持复制目录
 * @param cmd_share_var 共享数据
 * @param client_args 客户端参数
 * @return int 成功返回0，失败返回-1
 */
int cmd_cp(CMDShareVar *cmd_share_var, char *client_args)
{
    // 备份客户端发来的有关参数信息
    char back_up1_client_args[1024] = {};
    char back_up2_client_args[1024] = {};
    strcpy(back_up1_client_args, client_args);
    strcpy(back_up2_client_args, client_args);
    char select_sql[300];
    MYSQL_RES *res;
    MYSQL_ROW row;

    // 存储要发给客户端的所有信息，包括成功和错误的信息
    char to_client_info[1024] = {};

    const char *delim = " ";
    char *saveptr;
    // 存储分割后的第一个字符串，用户相对路径
    char *token = strtok_r(client_args, delim, &saveptr);

    // 存储分割后的第二个字符串，想要复制的文件的路径
    char *arg1 = strtok_r(NULL, delim, &saveptr);

    // 用户只输入了cp, 错误信息
    if (arg1 == NULL)
    {
        sprintf(to_client_info, "%s %s", token, "cp命令缺少操作对象");
        SendN(to_client_info, strlen(to_client_info), cmd_share_var->client_fd);
        return -1;
    }

    // 判断第一个参数是否存在, 且必须是文件（必须存在，否则非法）
    bzero(select_sql, sizeof(select_sql));
    sprintf(select_sql, "select file_path, file_type from file_info where user_id = %d and file_path = '%s' and delete_flag = 1", cmd_share_var->user_id, arg1);
    select_sql_file(cmd_share_var, select_sql, &res);
    row = mysql_fetch_row(res);

    // 错误信息，用户输入的参数不存在
    if (res == NULL || mysql_num_rows(res) == 0)
    {
        sprintf(to_client_info, "%s %s", token, "cp命令缺少操作对象");
        SendN(to_client_info, strlen(to_client_info), cmd_share_var->client_fd);
        return -1;
    }
    // 错误信息，第一个参数是目录
    if (atoi(row[1]) == 0)
    {
        sprintf(to_client_info, "%s %s", token, "cp命令第一个参数是目录, 不支持对目录的复制");
        SendN(to_client_info, strlen(to_client_info), cmd_share_var->client_fd);
        return -1;
    }

    // 存储分割后的第三个字符串
    char *arg2 = strtok_r(NULL, delim, &saveptr);
    // 用户只输入了cp和第一个参数, 错误信息
    if (arg2 == NULL)
    {
        sprintf(to_client_info, "%s %s", token, "cp命令缺少操作对象");
        SendN(to_client_info, strlen(to_client_info), cmd_share_var->client_fd);
        return -1;
    }

    // 存储分割后的第四个字符串
    char *arg3 = strtok_r(NULL, delim, &saveptr);

    // 判断分割后的第三个参数是否为空
    if (arg3 == NULL)
    {
        if (arg2 is dir)
        {
            cmd_cp_dir(cmd_share_var, arg1, arg2);
        }
        else
        {
            cmd_cp_file(cmd_share_var, arg1, arg2);
        }
    }
    if (arg3 != NULL)
    {
        // TODO: 不为空，多参数复制，进行两次循环切割
        token = strtok_r(back_up1_client_args, delim, &saveptr);
        char *last_token = NULL;
        // 遍历找到最后一个参数
        while (token != NULL)
        {
            last_token = token; // 更新last_token为当前的token
            printf("Token: %s\n", last_token);
            token = strtok_r(NULL, delim, &saveptr);
        }
        // 判断最后一个参数是否是已经存在的目录，否则非法
        bzero(select_sql, sizeof(select_sql));
        sprintf(select_sql, "select file_path, file_type from file_info where delete_flag = 1 and user_id = %d and file_path = '%s'", cmd_share_var->user_id, arg1);
        select_sql_file(cmd_share_var, select_sql, &res);
        row = mysql_fetch_row(res);
        // 查询结果不存在，存在但不是目录
        if ((res == NULL || mysql_num_rows(res) == 0) || atoi(row[1]) == 1)
        {
            // 错误信息，用户输入的参数不存在
            sprintf(to_client_info, "%s %s", token, "cp命令最后一个参数必须是已经存在的目录");
            SendN(to_client_info, strlen(to_client_info), cmd_share_var->client_fd);
            mysql_free_result(res);
            return -1;
        }

        // TODO:参数合法，递归将前面的所有参数复制到最后一个参数（目录）下，是文件就复制，目录就跳过，复制下一个参数
        token = strtok_r(back_up2_client_args, delim, &saveptr);
        while ((token = strtok_r(NULL, delim, &saveptr)) != NULL)
        {
            bzero(select_sql, sizeof(select_sql));
            sprintf(select_sql, "select file_path, file_type from file_info where user_id = %d and file_path = '%s' and delete_flag = 1 and file_type = 1", cmd_share_var->user_id, token);
            select_sql_file(cmd_share_var, select_sql, &res);
            row = mysql_fetch_row(res);
            if(res ==NULL || mysql_num_row(res)){
                
            }
            else{

            }
            // 每次切割都进行复制
            // copy(arg[i], last_token);
        }
    }
}

/**
 * @brief mv命令函数，移动或重命名网盘中文件到指定路径
 * @param cmd_share_var 共享数据
 * @param client_args 客户端参数
 * @return int 成功返回0，失败返回-1
 */
/*int cmd_mv(CMDShareVar *cmd_share_var, char *client_args)
{
    cmd_cp(cmd_share_var, client_args);

    // 备份客户端发来的有关参数信息
    char back_up1_client_args[1024] = {};
    char back_up2_client_args[1024] = {};
    strcpy(back_up1_client_args, client_args);
    strcpy(back_up2_client_args, client_args);

    // 存储要发给客户端的所有信息，包括成功和错误的信息
    char to_client_info[1024] = {};

    const char *delim = " ";
    char *saveptr;
    // 存储分割后的第一个字符串，用户相对路径
    char *token = strtok_r(client_args, delim, &saveptr);
    // 存储分割后的第二个字符串，想要移动位置的文件或目录的路径
    char *arg1 = strtok_r(NULL, delim, &saveptr);
    if (arg1 == NULL)
    { // 用户只输入了mv
        // 错误信息
        sprintf(to_client_info, "%s %s", token, "mv命令缺少操作对象");
        SendN(to_client_info, strlen(to_client_info), cmd_share_var->client_fd);
        return -1;
    }

    // TODO: 判断第一个参数是否存在（必须存在，否则非法）
    char select_sql[300] = {0};
    sprintf(select_sql, "select file_path from file_info where user_id = %d and file_path = '%s'", cmd_share_var->user_id, arg1);
    mysql_query(cmd_share_var->mysql, select_sql);
    MYSQL_RES *res = mysql_store_result(cmd_share_var->mysql);
    if (res == NULL || mysql_num_rows(res) == 0)
    {
        // 错误信息，用户输入的参数不存在
        sprintf(to_client_info, "%s %s", token, "mv命令缺少操作对象");
        SendN(to_client_info, strlen(to_client_info), cmd_share_var->client_fd);
        mysql_free_result(res);
        return -1;
    }

    // 存储分割后的第三个字符串，移动后的存储目录
    char *arg2 = strtok_r(NULL, delim, &saveptr);
    if (arg2 == NULL)
    { // 用户只输入了mv和第一个参数
        // 错误信息
        sprintf(to_client_info, "%s %s", token, "mv命令缺少操作对象");

        SendN(to_client_info, strlen(to_client_info), cmd_share_var->client_fd);
        mysql_free_result(res);
        return -1;
    }
    // TODO: 判断第一个参数是目录的情况下，第二个参数类型（是文件则非法

    // 存储分割后的第四个字符串
    char *arg3 = strtok_r(NULL, delim, &saveptr);

    // 判断分割后的第三个参数是否为空
    if (arg3 == NULL)
    {
        // TODO: 为空，对两个参数的正常移动，大多数操作
    }
    else
    {
        // TODO: 不为空，多参数移动，进行两次循环切割
        token = strtok_r(back_up1_client_args, delim, &saveptr);
        char *last_token = NULL;
        // 遍历找到最后一个参数
        while (token != NULL)
        {
            last_token = token; // 更新last_token为当前的token
            printf("Token: %s\n", last_token);
            token = strtok_r(NULL, delim, &saveptr);
        }
        // 判断最后一个参数是否是已经存在的目录，否则非法
        if (last_token != exist_dir)
        {
            // 命令输入错误
        }
        // TODO:参数合法，递归对前面的所有参数进行移动
        int err_check = 0;
        while()
        {

        }
        if(err_check ==1)
        {   //这里打印有部分参数没有被复制
            sprintf()
        }else{
            // 这里打印成功移动
        }
    }
}
*/
