import subprocess
import os 

def upload_file():
    bashCommand = "scp -i ~/.ssh/face ./index root@142.93.157.242:/var/www/html/index"
    process = subprocess.Popen(bashCommand.split(), stdout=subprocess.PIPE)
    output, error = process.communicate()
    
    bashCommand = "scp -i ~/.ssh/face -rp ./face_database root@142.93.157.242:/var/www/html/"
    process = subprocess.Popen(bashCommand.split(), stdout=subprocess.PIPE)
    output, error = process.communicate()
