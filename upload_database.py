import ftplib
​
def upload_face_db():
	session = ftplib.FTP('ge0rges.com','face','face')
	session.cwd("/var/www/ge0rges.com/html/face_database")
	​
	# Change the for loop:
	for file_name in dir:
	    file = open(file_name,'rb')                  # file to send
	    session.storbinary(file_name, file)     # send the file
	    file.close()                                    # close file and FTP
	session.quit()
	Collapse