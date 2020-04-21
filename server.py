import time
from http.server import BaseHTTPRequestHandler, HTTPServer
import cgi
import os
import urllib

from os import listdir
from os.path import isfile, join

HOST_NAME = '0.0.0.0' 
PORT_NUMBER = 8088

videopath = '/sdcard/videos/'
datastorage= []

class MyHandler(BaseHTTPRequestHandler):
    def do_HEAD(s):
        #print("header");
        s.send_response(200)
        s.send_header("Content-type", "text/html")
        s.end_headers()

    def do_POST(s):
        s.send_response(200)
        s.send_header("Content-type", "text/html")
        s.end_headers()
        s.wfile.write(bytes('<html><head><title>Video Deleted OK</title></head><body></body></html>','utf-8'))
        s.wfile.write(bytes("<a href='/'>Back to overview</a>", 'utf-8'))

#        print(s.headers)
        contentLengthAsList = s.headers.get('Content-Length')
        contentLengthAsInt = int(contentLengthAsList)
        print(contentLengthAsInt)
        x = urllib.parse.parse_qs(s.rfile.read(contentLengthAsInt), keep_blank_values=1)
#        print(x)
        file_to_delete = x[b'indx'][0]
        print(file_to_delete.decode("utf-8"))
        os.remove(videopath + "/"+ file_to_delete.decode("utf-8"))



    def do_GET(s):
        video = s.path.strip('\/\'')
        if (s.path != '/'):
            #print("NON root request - "+'../' + video)
            s.send_response(200)
            s.send_header('Content-type', 'application/octet-stream')
            with open('/sdcard/videos/' + video, 'rb') as file: 
                fsize = str(os.fstat(file.fileno())[6])
                #print(fsize)
                s.send_header('Content-Length',fsize)
                s.end_headers()
                s.wfile.write(file.read())
            return


        datastorage = [f for f in listdir(videopath) if isfile(join(videopath, f))]
        """Respond to a GET request."""
        s.send_response(200)
        s.send_header("Content-type", "text/html")
        s.end_headers()
        s.wfile.write(b'<html><head><title>OpenPilot Dashcam Videos</title></head>')
        s.wfile.write(b'<body><p>Stored Videos</p>')
        for x in range(len(datastorage)):
            st = '<form action="/" method="POST"><a href="'+datastorage[x]+'">'+datastorage[x]+'</a><input type="hidden" name="indx" value="'+datastorage[x]+'"><input type="hidden" name="_method" value="DELETE"/><input type="submit" value="Remove"/></form></br>'
            s.wfile.write(bytes(st, 'utf-8'))

        s.wfile.write(b'</body></html>')

    def do_DELETE(s):
        global datastorage
        print('That is working')
        contentLengthAsList = s.headers.get('content-length')
        contentLengthAsInt = int(contentLengthAsList[0])

        x = cgi.parse_qs(s.rfile.read(contentLengthAsInt), keep_blank_values=1)
        datastorage = datastorage.pop(x)

        s.send_response(301) #Moved Permanently
        s.send_header("location", "/")
        s.end_headers()



if __name__ == '__main__':
     server_class = HTTPServer
     httpd = server_class((HOST_NAME, PORT_NUMBER), MyHandler)
     print(time.asctime(), "Server Starts - %s:%s" % (HOST_NAME, PORT_NUMBER))
     try:
          httpd.serve_forever()
     except KeyboardInterrupt:
          pass
     httpd.server_close()
     print(time.asctime(), "Server Stops - %s:%s" % (HOST_NAME, PORT_NUMBER))
