import pexpect

child = pexpect.spawn('scp -rp ./known_ppl face@ge0rges.com:/var/www/ge0rges.com/html/face_database')
child.expect("Password:")
child.sendline("face")

# Create index file
# Write to index file at ./index

child = pexpect.spawn('scp -rp ./index face@ge0rges.com:/var/www/ge0rges.com/html/face_database/index')
child.expect("Password:")
child.sendline("face")
