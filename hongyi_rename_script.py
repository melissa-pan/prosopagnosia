import os
from flask import Flask, request
import json

def rename_file(name_dict,path):
    for key in name_dict:
        print(key, name_dict[key])
        if not os.path.exists(path+key):
            print("file " + path+key + " not found ")
            continue
        try:
            os.rename(path+key , path+name_dict[key])
            print("rename succeed")
        except Exception as e:
            print("rename fail: "+path+key)
            print(e)

def build_dict_mapping(json_inMem):
    f = open("dictionary_mapping","w")
    f.write(json.dumps(json_inMem))
    f.close()

app = Flask(__name__)
@app.route('/',methods=['POST'])
def get_json_dict():
    data = request.json
    rename_file(data, "")
    build_dict_mapping(data)
    return ("good")

if __name__ == "__main__":
    app.run("localhost", "5000",debug=True)