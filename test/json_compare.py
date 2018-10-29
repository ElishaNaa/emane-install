import json
import sys
import types

TYPE = 'TYPE'
PATH = 'PATH'
VALUE = 'VALUE'


class Diff(object):
    def __init__(self, first, second, with_values=False):
        self.difference = []
        self.seen = []
        not_with_values = not with_values
        self.check(first, second, with_values=with_values)

    def check(self, first, second, path='', with_values=False):
        if with_values and second != None:
            if not isinstance(first, type(second)):
                message = '%s - %s, %s' % (path, type(first).__name__, type(second).__name__)
                self.save_diff(message, TYPE)

        if isinstance(first, dict):
            for key in first:
                # the first part of path must not have trailing dot.
                if len(path) == 0:
                    new_path = key
                else:
                    new_path = "%s.%s" % (path, key)

                if isinstance(second, dict):
                    if second.has_key(key):
                        sec = second[key]
                    else:
                        #  there are key in the first, that is not presented in the second
                        self.save_diff(new_path, PATH)

                        # prevent further values checking.
                        sec = None

                    # recursive call
                    if sec != None:
                        self.check(first[key], sec, path=new_path, with_values=with_values)
                else:
                    # second is not dict. every key from first goes to the difference
                    self.save_diff(new_path, PATH)
                    self.check(first[key], second, path=new_path, with_values=with_values)

        # if object is list, loop over it and check.
        elif isinstance(first, list):
            for (index, item) in enumerate(first):
                new_path = "%s[%s]" % (path, index)
                # try to get the same index from second
                sec = None
                if second != None:
                    try:
                        sec = second[index]
                    except (IndexError, KeyError):
                        # goes to difference
                        self.save_diff('%s - %s' % (new_path, type(item).__name__), TYPE)

                # recursive call
                self.check(first[index], sec, path=new_path, with_values=with_values)

        # not list, not dict. check for equality (only if with_values is True) and return.
        else:
            if with_values and second != None:
                if first != second:
                    self.save_diff('%s - %s | %s' % (path, first, second), VALUE)
            return

    def save_diff(self, diff_message, type_):
        if diff_message not in self.difference:
            self.seen.append(diff_message)
            self.difference.append((type_, diff_message))


def getContentFromFile(filePath):
    return open(filePath, 'r').read()


def getContent(location):
    content = None
    if type(location) is types.DictType:
        return location
    content = getContentFromFile(location)
    if content is None:
        raise Error("Could not load content for " + location)
    return json.loads(content)


def compare(location1, location2):
    json1 = getContent(location1)
    json2 = getContent(location2)
    json1 = sorted(json1)
    json2 = sorted(json2)
    diff1 = Diff(json1, json2, True).difference
    diff2 = Diff(json2, json1, False).difference
    diffs = []
    for type, message in diff1:
        newType = 'CHANGED'
        if type == PATH:
            newType = 'REMOVED'
        diffs.append({'type': newType, 'message': message})
    for type, message in diff2:
        diffs.append({'type': 'ADDED', 'message': message})
    return diffs


if __name__ == '__main__':
  '''
  python json_compare json1 (path of file json1) json2 (path of file json2) key (path of file key)

  '''
    if len(sys.argv) != 4:
        sys.exit('Error')
    location1 = sys.argv[1]
    keys = []
    with open(sys.argv[3], 'r') as f:
        lines = f.readlines()
        for key in lines:
            if '\n' in key:
                key = key[:-1]
            keys.append(key)

    location2 = sys.argv[2]
    diffs = compare(location1, location2)
    if len(diffs) > 0:
        #print '\r\nFound differences comparing ' + location1 + ' and ' + location2
    #for diff in diffs:
        #print diff['type'] + ': ' + diff['message']
        for diff in diffs:
            if str(diff['type']) == ("CHANGED" or "REMOVED"):
                print 'False'
                break
            else:
                keys = sorted(keys)
                res = []
                for diff in diffs:
                    res.append(str(diff['message']))
                res = sorted(res)
                if len(res) < len(keys) or res != keys:
                    print 'False'
                    break
                else:
                    print 'True'
                    break
    else:
        print 'True'

