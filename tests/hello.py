import bjoern
from random import choice

def app1(env, sr):
    sr(b'200 ok', [(b'Foo', b'Bar'), (b'Blah', b'Blubb'), (b'Spam', b'Eggs'), (b'Blurg', b'asdasjdaskdasdjj asdk jaks / /a jaksdjkas jkasd jkasdj '),
                  (b'asd2easdasdjaksdjdkskjkasdjka', b'oasdjkadk kasdk k k k k k ')])
    return [b'hello', b'world']

def app2(env, sr):
    sr(b'200 ok', [])
    return b'hello'

def app3(env, sr):
    sr(b'200 abc', [(b'Content-Length', b'12')])
    yield b'Hello'
    yield b' World'
    yield b'\n'

def app4(e, s):
    s(b'200 ok', [])
    return [b'hello\n']

apps = (app1, app2, app3, app4)

def wsgi_app(env, start_response):
    return choice(apps)(env, start_response)

if __name__ == '__main__':
    bjoern.run(wsgi_app, '0.0.0.0', 8080)
