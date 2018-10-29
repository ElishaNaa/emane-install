import json
import redis
import sys
#import datetime
import ast

def snapshot(redis_ip, filePath, hashtable):
    pool = redis.ConnectionPool(host=redis_ip, port=6379, db=0)
    r = redis.Redis(connection_pool=pool)
    arryVals = r.hvals(hashtable)
    arryDict = []
    for val in arryVals:
        arryDict.append(ast.literal_eval(val))
    with open(filePath, 'ab') as outfile:
        json.dump(arryDict, outfile)


if __name__ == '__main__':
    '''
    python redis_snapshot redis_ip (e.g 10.99.1.host_id) file path (e.g con_nemid) hashtable (e.g CON_MAP_11)
    '''
    snapshot(sys.argv[1], sys.argv[2], sys.argv[3])
