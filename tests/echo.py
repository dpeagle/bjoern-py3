def app(e, s):
    print(e)
    s('200 ok', [])
    return e["wsgi.input"].readline()

import bjoern
bjoern.run(app, '0.0.0.0', 8080)
