/*config_operation.c*/
#include <stdio.h>
#include <string.h>

#include "config_operation.h"

// #pragma warning(disable:4996)

int append_ini_key_string(const char* title, const char* key, char* val, const char* filename)
{
    int iFound = -1;
    FILE* fpr, * fpw;
    char sLine[1024], sTitle[32], * cEqualPtr;
    char sNewLine[1024];
    sprintf(sTitle, "[%s]", title);
    if (NULL == (fpr = fopen(filename, "r")))
    {
		printf("PARAM_Save read %s failed\n", filename);
		return -1;// 读取原文件
    }
    sprintf(sLine, "%s.tmp", filename);
    if (NULL == (fpw = fopen(sLine, "w")))
    {
		printf("PARAM_Save write %s failed\n", sLine);
        return -1;// 写入临时文件
    }
    while (NULL != fgets(sLine, 1024, fpr)) {
        cEqualPtr = strchr(sLine, '=');
        if (0 == strncmp(sTitle, sLine, strlen(sTitle))) { // 长度依文件读取的为准
            iFound = 0;  // 找到标题位置
        }
        // 发现一个新标题。并且在指定的标题中未找到同名的KEY。
		else if((sLine[0] == '[') && (iFound == 0))
		{
            iFound = 1;
            sprintf(sNewLine, "%s = %s\n", key, val);
        	fputs(sNewLine, fpw); // 写入临时文件
		}
        else if ((NULL != cEqualPtr) && (0 == iFound)) {
            // Duplicate Key
            if (0 == strncmp(key, sLine, strlen(key))) {
                printf("Duplicate Key\n");
                iFound = -1;
                break;
            }
        }
		fputs(sLine, fpw); // 写入临时文件
    }
	if(iFound == 0)  // 传入的标题就是文件中的最后一个标题。
	{
        iFound = 1;
        sprintf(sNewLine, "%s = %s\n", key, val);
    	fputs(sNewLine, fpw); // 写入临时文件
	}
    fclose(fpr);
    fclose(fpw);
    sprintf(sLine, "%s.tmp", filename);
    if (iFound == 1)
    {
        remove(filename);         // 删除原文件
		printf("PARAM_Save remove %s\n", filename);
        rename(sLine, filename);  // 将临时文件更新到原文件
		printf("PARAM_Save rename %s to %s\n", filename, sLine);
    }
    return iFound;
}


int delete_ini_key_string(const char* title, const char* key, const char* filename)
{
    int iFound = -1;
    FILE* fpr, * fpw;
    char sLine[1024], sTitle[32], * cEqualPtr;
    sprintf(sTitle, "[%s]", title);
    if (NULL == (fpr = fopen(filename, "r")))
    {
		printf("PARAM_Save read %s failed\n", filename);
		return -1;// 读取原文件
    }
    sprintf(sLine, "%s.tmp", filename);
    if (NULL == (fpw = fopen(sLine, "w")))
    {
		printf("PARAM_Save write %s failed\n", sLine);
        return -1;// 写入临时文件
    }
    while (NULL != fgets(sLine, 1024, fpr)) {
        cEqualPtr = strchr(sLine, '=');
        if (0 == strncmp(sTitle, sLine, strlen(sTitle))) { // 长度依文件读取的为准
            iFound = 0;  // 找到标题位置
        }
        else if ((NULL != cEqualPtr) && (0 == iFound)) {
            // Found Key
            if (0 == strncmp(key, sLine, strlen(key))) {
                printf("Found Key\n");
                iFound = 1;  // 找到KEY
                continue;
            }
        }
		fputs(sLine, fpw); // 写入临时文件
    }
    fclose(fpr);
    fclose(fpw);
    sprintf(sLine, "%s.tmp", filename);
    if (iFound == 1)
    {
        remove(filename);         // 删除原文件
		printf("PARAM_Save remove %s\n", filename);
        rename(sLine, filename);  // 将临时文件更新到原文件
		printf("PARAM_Save rename %s to %s\n", filename, sLine);
    }
    return iFound;
}

/***********************************************************************
 * 函数名：         get_ini_key_string
 * 入口参数：         title: 配置文件中一组数据的标识
 *                       key: 这组数据中要读出的值的标识
 *                  filename: 要读取的文件路径
 * 返回值：         找到需要查的值则返回正确结果 0
 *                  否则返回-1
 ***********************************************************************/
int get_ini_key_string(const char* title, const char* key, char* bufValue, const char* filename)
{
    FILE* fp;
    int  flag = 0;
    char sTitle[64], * cEqualPtr;
    char sLine[1024];
    sprintf(sTitle, "[%s]", title);

    if (NULL == (fp = fopen(filename, "r"))) {
        perror("get_ini_key_string fopen");
        return -1;
    }
    while (NULL != fgets(sLine, 1024, fp)) {
        // 这是注释行
        if (0 == strncmp("//", sLine, 2)) continue;
        if ('#' == sLine[0])              continue;
        cEqualPtr = strchr(sLine, '=');
        if ((NULL != cEqualPtr) && (1 == flag)) {
            if (0 == strncmp(key, sLine, strlen(key))) { // 长度依文件读取的为准
                sLine[strlen(sLine) - 1] = '\0';
                fclose(fp);
                while (*(cEqualPtr + 1) == ' ') {
                    cEqualPtr++;
                }
                strcpy(bufValue, cEqualPtr + 1);
                return 0;
            }
        }
        else {
            if (0 == strncmp(sTitle, sLine, strlen(sTitle))) { // 长度依文件读取的为准
                flag = 1; // 找到标题位置
            }
            // 发现一个新标题。并且在指定的标题中未找到同名的KEY。
            else if((sLine[0] == '[') && (flag == 1))
            {
                flag = 0;
            }
        }
    }
    fclose(fp);
    return -1;
}

/***********************************************************************
 * 函数名：         set_ini_key_string
 * 入口参数：         title: 配置文件中一组数据的标识
 *                       key: 这组数据中要读出的值的标识
 *                       val: 更改后的值
 *                  filename: 要读取的文件路径
 * 返回值：         成功返回  0
 *                  否则返回 -1
 ***********************************************************************/
int set_ini_key_string(const char* title, const char* key, char* val, const char* filename)
{
    int iFound = -1;
    FILE* fpr, * fpw;
    int  flag = 0;
    char sLine[1024], sTitle[32], * cEqualPtr;
    sprintf(sTitle, "[%s]", title);
    if (NULL == (fpr = fopen(filename, "r")))
    {
		printf("PARAM_Save read %s failed\n", filename);
		return -1;// 读取原文件
    }
    sprintf(sLine, "%s.tmp", filename);
    if (NULL == (fpw = fopen(sLine, "w")))
    {
		printf("PARAM_Save write %s failed\n", sLine);
        return -1;// 写入临时文件
    }
    while (NULL != fgets(sLine, 1024, fpr)) {
        if (2 != flag) { // 如果找到要修改的那一行，则不会执行内部的操作
            cEqualPtr = strchr(sLine, '=');
            if ((NULL != cEqualPtr) && (1 == flag)) {
                if (0 == strncmp(key, sLine, strlen(key))) { // 长度依文件读取的为准
                    flag = 2;// 更改值，方便写入文件
                    sprintf(cEqualPtr + 1, " %s\n", val);
                    iFound = 0;
					printf("PARAM_Save write %s = %s\n", key, val);
                }
            }
            else {
                if (0 == strncmp(sTitle, sLine, strlen(sTitle))) { // 长度依文件读取的为准
                    flag = 1; // 找到标题位置
                }
            }
        }
        fputs(sLine, fpw); // 写入临时文件
    }
    fclose(fpr);
    fclose(fpw);
    sprintf(sLine, "%s.tmp", filename);
    if (iFound == 0)
    {
        remove(filename);         // 删除原文件
		printf("PARAM_Save remove %s\n", filename);
        rename(sLine, filename);  // 将临时文件更新到原文件
		printf("PARAM_Save rename %s to %s\n", filename, sLine);
    }
    return iFound;
}

static int generate_ini_file(const char* title, const char* keyPrefix, int iKeyCount, int isCreateFile, const char* filename)
{
    FILE* fp;
    if (isCreateFile == 1)
    {
        fp = fopen(filename, "w");
    }
    else {
        fp = fopen(filename, "a");
    }
    if (fp == NULL)
    {
        perror("generate_ini_file fopen");
        return -1;
    }

    int i = 0;
    fprintf(fp, "[%s]\n", title);
    for (i = 0; i < iKeyCount; i++) {
        fprintf(fp, "%s%03d = 0\n", keyPrefix, i);
        //printf("%s=%s\n",items[i].key, items[i].value);
    }
    fclose(fp);
    return 1;
}

int create_keyvalue_in_inifile(const char* title, const char* key, const char* value, const char* filename) {
    FILE* fp;
    fp = fopen(filename, "w");
	
    if (fp == NULL)
    {
        perror("create_keyvalue_in_inifile fopen");
        return -1;
    }
	fprintf(fp, "[%s]\n", title);
    fprintf(fp, "%s = %s\n", key, value);
    fclose(fp);
    return 1;
}

int append_keyvalue_in_inifile(const char* key, const char* value, const char* filename) {
    FILE* fp;
	fp = fopen(filename, "a");
	
    if (fp == NULL)
    {
        perror("append_keyvalue_in_inifile fopen");
        return -1;
    }
    fprintf(fp, "%s = %s\n", key, value);
    fclose(fp);
    return 1;
}

int create_ram_in_inifile(const char* title, int iKeyCount, const char* filename) {
    return generate_ini_file(title, "Config", iKeyCount, 1, filename);
}

int append_ram_in_inifile(const char* title, int iKeyCount, const char* filename) {
    return generate_ini_file(title, "Config", iKeyCount, 0, filename);
}