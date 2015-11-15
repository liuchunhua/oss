#include <string.h>
#include <stdlib.h>

#include "List.h"
#include "HashTable.h"
#include "String.h"
#include "ossutil.h"
#include "log.h"

static void sort_list_asc(List ls)
{
    log_debug("%d", ls == NULL);
    if (ListClass.isEmpty(ls))
    {
        log_debug("list is empty");
        return;
    }
    log_debug("排序开始");
    List last = ls;
    while (ls->next != last)
    {
        List node = ls->next;
        List next = node->next;
        while (next != last)
        {
            struct pair *a = (struct pair*) node->ptr;
            struct pair *b = (struct pair*) next->ptr;
            char *a_key = (char*) a->key;
            char *b_key = (char*) b->key;
            if (strcoll(a_key, b_key) > 0)
            {
                node->prev->next = node->next;
                node->next = next->next;
                next->prev = node->prev;
                node->prev = next;
                next->next = node;
                next = node->next;
            }
            if (strcoll(a_key, b_key) == 0)
            {
                int size = strlen((char*) a->value) + strlen((char*) b->value)
                        + 2;
                char *new_string = malloc(size);
                strcat(new_string, (char*) a->value);
                strcat(new_string, ",");
                strcat(new_string, (char*) b->value);
                free(a->value);
                a->value = (void*) new_string;
                free(b->key);
                free(b->value);
                free(b);
                listDel(next);
                next = node->next;
            }
            else
            {
                node = next;
                next = next->next;
            }
        }
        last = last->prev;
    }
}

static char *to_string(List list)
{
    if (ListClass.isEmpty(list))
        return NULL ;

    int len = 0;
    List node = NULL;
    for_each(node, list)
    {
        struct pair *pair = node->ptr;
        char *key = pair->key;
        char *value = pair->value;
        len += strlen(key);
        len += strlen(value);
        len += 2; //":" "\n"
    }

    char *mem = malloc(len);
    memset(mem, 0x0, len);

    node = NULL;
    for_each(node, list)
    {
        if (strlen(mem) != 0)
            strcat(mem, "\n");
        struct pair *pair = node->ptr;
        char *key = pair->key;
        char *value = pair->value;
        strcat(mem, key);
        strcat(mem, ":");
        strcat(mem, value);
    }

    return mem;
}

char *canonicalizedOSSHeaders(HashTable *headers)
{
    List list = HashTableClass.get_all(headers);
    //<struct pair>
    HashTable *table = hash_table_init();

    if (table == NULL )
    {
        ListClass.destroy(list);
    }

    //获得以x-oss开头的head
    List node;
    for_each(node, list)
    {
        struct pair * pair = node->ptr;
        char *key = pair->key;
        log_debug("header[%s]", key);
        if (strstr(key, "x-oss-") != NULL)
        {
            struct pair *header = HashTableClass.get(table, key);
            if (header == NULL )
            {
                HashTableClass.put(table, key, strdup(pair->value));
            }
            else
            {
                char *temp = header->value;
                header->value = StringClass.concat(3, temp, ",", pair->value);
                free((void *) temp);
            }
        }
    }
    List oss = HashTableClass.get_all(table);
    log_debug("排序");
    sort_list_asc(oss);
    char * str = to_string(oss);

    ListClass.destroy(oss);
    ListClass.destroy(list);
    HashTableClass.destroy_fun(table, free);

    return str;
}

static char *to_res_string(List list)
{
    if (ListClass.isEmpty(list))
        return NULL ;
    int i = 0;
    int len = 0;
    List node = NULL;
    for_each(node, list)
    {
        char *str = node->ptr;
        len += strlen(str);
        len += 1; //"&"
    }

    char *mem = malloc(len);
    memset(mem, 0x0, len);
    strcat(mem, list->next->ptr);
    free(list->next->ptr);
    ListClass.del(list->next);

    node = NULL;
    for_each(node, list)
    {
        if (i == 0)
            strcat(mem, "?");
        else
            strcat(mem, "&");
        char *str = node->ptr;
        strcat(mem, str);
        i++;
    }

    return mem;
}

char *canonicalizedResource(const char * resource)
{
    char *resources[] =
    { "acl", "group", "loggine", "partNumber", "uploadId", "uploads" };
    char *override[] =
    { "response-content-type", "response-content-language", "response-expires",
            "reponse-cache-control", "response-content-disposition",
            "response-content-encoding" };

    List list = ListClass.init();

    char *res = strchr(resource, '?');
    if (res == NULL )
        return strdup(resource);

    int n = StringClass.indexOf(resource, '?');

    ListClass.add(list, StringClass.substring(resource, 0, n - 1));
    while (res != NULL )
    {
        n = StringClass.indexOf(res, '&');
        char *subresource = StringClass.substring(res, 0, n - 1);
        int i = 0;
        for (; i < sizeof(resources) / sizeof(char *); i++)
        {
            if (strstr(subresource, resources[i]) != NULL )
            {
                ListClass.add(list, strdup(subresource));
                break;
            }
        }
        for (i = 0; i < sizeof(override) / sizeof(char *); i++)
        {
            if (strstr(subresource, override[i]) != NULL )
            {
                ListClass.add(list, strdup(subresource));
                break;
            }
        }
        free(subresource);
        res = strstr(res, "&");
    }

    res = to_res_string(list);
    ListClass.destroy_fun(list, free);
    return res;
}

char *oss_authorizate(const char *key, const char *method, HashTable *headers,
        const char* resource)
{
    log_debug("验证开始");
    char *date = (char *) HashTableClass.get(headers, "Date");
    char *md5 = (char *) HashTableClass.get(headers, "Content-Md5");
    char *content_type = (char *) HashTableClass.get(headers, "Content-Type");
    log_debug("获取计算的x-oss");
    char *oss_headers = canonicalizedOSSHeaders(headers);
    log_debug("header:%s", oss_headers);
    log_debug("获取计算的子资源");
    char *res = canonicalizedResource(resource);
    log_debug("resource: %s", res);
    char *buf = malloc(1024 * 4);
    memset(buf, 0x0, 1024 * 4);
    strcat(buf, method);
    strcat(buf, "\n");
    log_debug("%s", buf);
    if (md5)
    {
        strcat(buf, md5);
    }
    strcat(buf, "\n");
    if (content_type)
    {
        strcat(buf, content_type);
    }
    strcat(buf, "\n");
    if (date)
    {
        strcat(buf, date);
        strcat(buf, "\n");
    }
    if (oss_headers)
    {
        strcat(buf, oss_headers);
        strcat(buf, "\n");
    }
    if (res)
    {
        strcat(buf, res);
    }
    log_debug("计算的字段： %s", buf);
    char *auth = hmac_base64(buf, strlen(buf), key, strlen(key));

    log_debug("Authorization : %s", auth);
    free(buf);
    if (oss_headers)
        free(oss_headers);
    if (res)
        free(res);

    return auth;
}
//resource中不包含bucket,所以计算时需要在resource前加/bucket
static char *new_canonicalizedResource(OSSPtr oss, const char * resource)
{
    char *resources[] =
    { "acl", "group", "loggine", "partNumber", "uploadId", "uploads" };
    char *override[] =
    { "response-content-type", "response-content-language", "response-expires",
            "reponse-cache-control", "response-content-disposition",
            "response-content-encoding" };
    char *res = NULL;
    if (oss->bucket)
    {
        res = StringClass.concat(3, "/", oss->bucket, resource);
    }
    else
        res = strdup(resource);
    log_debug("url : %s", res);
    if (strchr(res, '?') == NULL )
    {
        return res;
    }

    char *resOld = res;
    int n = StringClass.indexOf(res, '?');

    List list = ListClass.init();
    if(n > 0)
    {
        char *url = StringClass.substring(res, 0, n - 1);
        log_debug("%s", url);
        ListClass.add(list, StringClass.substring(res, 0, n - 1));
        res = strstr(res, "?") + 1;
    }
    while (res != NULL )
    {
        n = StringClass.indexOf(res, '&');
        char *subresource = NULL;
        if(n > 0)
            subresource = StringClass.substring(res, 0, n - 1);
        else
            subresource = strdup(res);
        if(!subresource) break;

        int i = 0;
        for (; i < sizeof(resources) / sizeof(char *); i++)
        {
            if (strstr(subresource, resources[i]) != NULL )
            {
                ListClass.add(list, strdup(subresource));
                break;
            }
        }
        for (i = 0; i < sizeof(override) / sizeof(char *); i++)
        {
            if (strstr(subresource, override[i]) != NULL )
            {
                ListClass.add(list, strdup(subresource));
                break;
            }
        }
        free(subresource);
        res = strstr(res, "&");
        if(res)
            res += 1;
    }

    res = to_res_string(list);
    ListClass.destroy_fun(list, free);
    if (oss->bucket)
        free(resOld);
    return res;
}

char *new_oss_authorizate(OSSPtr oss, const char *method, HashTable *headers,
        const char* resource)
{
    log_debug("验证开始");
    char *date = (char *) HashTableClass.get(headers, "Date");
    char *md5 = (char *) HashTableClass.get(headers, "Content-Md5");
    char *content_type = (char *) HashTableClass.get(headers, "Content-Type");
    log_debug("获取计算的x-oss");
    char *oss_headers = canonicalizedOSSHeaders(headers);
    log_debug("header:%s", oss_headers);
    log_debug("获取计算的子资源");
    char *res = new_canonicalizedResource(oss, resource);
    log_debug("resource: %s", res);
    char *buf = malloc(1024 * 4);
    memset(buf, 0x0, 1024 * 4);
    strcat(buf, method);
    strcat(buf, "\n");
    log_debug("%s", buf);
    if (md5)
    {
        strcat(buf, md5);
    }
    strcat(buf, "\n");
    if (content_type)
    {
        strcat(buf, content_type);
    }
    strcat(buf, "\n");
    if (date)
    {
        strcat(buf, date);
        strcat(buf, "\n");
    }
    if (oss_headers)
    {
        strcat(buf, oss_headers);
        strcat(buf, "\n");
    }
    if (res)
    {
        strcat(buf, res);
    }
    log_debug("计算的字段： %s", buf);
    char *auth = hmac_base64(buf, strlen(buf), oss->access_key,
            strlen(oss->access_key));

    log_debug("Authorization : %s", auth);
    free(buf);
    if (oss_headers)
        free(oss_headers);
    if (res)
        free(res);

    return auth;
}
