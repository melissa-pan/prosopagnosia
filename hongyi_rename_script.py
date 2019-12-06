import os
from flask import Flask, request

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

app = Flask(__name__)
@app.route('/',methods=['POST'])
def get_json_dict():
    data = request.json
    rename_file(data, "")
    return "Good!"
if __name__ == "__main__":
    app.run("localhost", "5000",debug=True)