import os
import sqlite3
import urllib2

# create and close a SQLite in-memory database
sqlite3.connect(':memory:').close()

pid = os.fork()
if pid == 0:
    # in the child, try to open a URL: it will crash
    print 'child', os.getpid(), 'calling urllib2.urlopen()'
    urllib2.urlopen('http://example.com/')
else:
    print 'parent forked child:', pid
    r = os.waitpid(pid, 0)
    print 'child exited status:', r[1]
    print 'WIFEXITED?', os.WIFEXITED(r[1]), 'WIFSIGNALED?', os.WIFSIGNALED(r[1])
