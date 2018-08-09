#include <sstream>
#include <fstream>
#include <list>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>


#include "txtparser.h"
#include "include/hiredis.h"

using namespace std;


int main(int argc, char **argv) {
    unsigned int j;
    redisContext *c;
    redisReply *reply;
    const char *hostname = (argc > 1) ? argv[1] : "10.99.0.100";
    int port = (argc > 2) ? atoi(argv[2]) : 6379;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    txtparser input;
    std::list<std::string> data = input.getData();
    for (list<std::string>::iterator iter = data.begin(); iter != data.end(); iter++)
    {
        const char *key = iter->c_str();
        printf("KEY:%s ", key);
        iter++;
        const char *val = iter->c_str();
        printf(" VAL:%s ", val);
        reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", key, val));
        printf("SET: %s \n", reply->str);
        freeReplyObject(reply);
    }



    // /* PING server */
    // reply = static_cast<redisReply*>(redisCommand(c,"PING"));
    // printf("PING: %s\n", reply->str);
    // freeReplyObject(reply);

    // /* Set a key */
    // reply = static_cast<redisReply*>(redisCommand(c,"SET %s %s", "foo", "hello world"));
    // printf("SET: %s\n", reply->str);
    // freeReplyObject(reply);

    // /* Set a key using binary safe API */
    // reply = static_cast<redisReply*>(redisCommand(c,"SET %b %b", "bar", (size_t) 3, "hello", (size_t) 5));
    // printf("SET (binary API): %s\n", reply->str);
    // freeReplyObject(reply);

    // /* Try a GET and two INCR */
    // reply = static_cast<redisReply*>(redisCommand(c,"GET foo"));
    // printf("GET foo: %s\n", reply->str);
    // freeReplyObject(reply);

    // reply = static_cast<redisReply*>(redisCommand(c,"INCR counter"));
    // printf("INCR counter: %lld\n", reply->integer);
    // freeReplyObject(reply);
    // /* again ... */
    // reply = static_cast<redisReply*>(redisCommand(c,"INCR counter"));
    // printf("INCR counter: %lld\n", reply->integer);
    // freeReplyObject(reply);

    // /* Create a list of numbers, from 0 to 9 */
    // reply = static_cast<redisReply*>(redisCommand(c,"DEL mylist"));
    // freeReplyObject(reply);
    // for (j = 0; j < 10; j++) {
    //     char buf[64];

    //     snprintf(buf,64,"%u",j);
    //     reply = static_cast<redisReply*>(redisCommand(c,"LPUSH mylist element-%s", buf));
    //     freeReplyObject(reply);
    // }

    // /* Let's check what we have inside the list */
    // reply = static_cast<redisReply*>(redisCommand(c,"LRANGE mylist 0 -1"));
    // if (reply->type == REDIS_REPLY_ARRAY) {
    //     for (j = 0; j < reply->elements; j++) {
    //         printf("%u) %s\n", j, reply->element[j]->str);
    //     }
    // }
    // freeReplyObject(reply);

    /* Disconnects and frees the context */
    redisFree(c);

    return 0;
}