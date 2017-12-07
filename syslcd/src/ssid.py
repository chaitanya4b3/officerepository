import random
import sqlite3
import string
import os
count=0
db = sqlite3.connect('/www/usbshare/udisk/config/vsys.db')
cursor = db.cursor()
cursor.execute('''SELECT MAC_ADDR FROM VALIDATE''')
all_books = cursor.fetchall()
os.system('rm /www/usbshare/udisk/config/SSID.db')
mac_id=[str(i[0]) for i in all_books]
print(mac_id)
ssid=[]
pwd=[]
def randomword_ssid(length):
   letters = string.ascii_letters +string.digits
   return ''.join(random.choice(letters) for i in range(length))
list1=randomword_ssid(31)
def randomword_pass(length):
 letters = string.digits
 return ''.join(random.choice(letters) for i in range(length))

for i in range(5):
 ssid.append(randomword_ssid(31)) 

for i in range(5):
 pwd.append(randomword_pass(9))
con = sqlite3.connect('/www/usbshare/udisk/config/SSID.db')

print "Opened database successfully";

#con.execute('''CREATE TABLE IF NOT EXISTS WIFI_TOKEN(ID integer PRIMARY KEY,MACID text NOT NULL,SSID text NOT NULL,PASSWORD text NOT NULL);''')

con.execute('''CREATE TABLE IF NOT EXISTS WIFI_TOKEN
         (ID INT NOT NULL,
         MACID           TEXT    NOT NULL,
         SSID            TEXT     NOT NULL,
         PASSWORD        TEXT NOT NULL);''')
for i in range(5):
 con.execute("INSERT INTO WIFI_TOKEN VALUES (?, ?, ?, ?);", (1, mac_id[i], ssid[i],pwd[i] ))
#print "Table created successfully";
con.commit()
con.close()

