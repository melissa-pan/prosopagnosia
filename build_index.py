import os
import json

def build_index():
    f = open("index", "w")
    f.write(json.dumps(os.listdir("./face_database")))
    f.close()
        