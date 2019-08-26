#
# Python script for WIZnet IoT iOffload Contest 2019
#
# Extract YouTube channel name and subscriber count from a YouTube user page
# and send it to an Arduino-Uno-based webserver for displaying the data on an LCD. 
#
# Copyright (C) 2019 Clemens Valens
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

import certifi
import urllib3
import time
import os

# Log program output to a CSV file.
log_file_name = "subscribers.csv"

# Three popular YouTube channels:
user1 = 'PewDiePie' # PewDiePie
user2 = 'tseries' # T-Series
user3 = 'ladygagaofficial' # Lady Gaga

# YouTube base URL:
youtube = 'https://www.youtube.com/user/'
# Construct the URIs for the channels we want to monitor.
uri1 = youtube + user1 # 'https://www.youtube.com/user/PewDiePie' #PewDiePie
uri2 = youtube + user2 # 'https://www.youtube.com/user/tseries' # T-Series
uri3 = youtube + user3 # 'https://www.youtube.com/user/ladygagaofficial' # Lady Gaga

# This is the tag that delimits the subscriber count section in the page.
# Of course, YouTube can decide to change it at any moment :-(
tag = 'yt-subscription-button-subscriber-count-branded-horizontal'

# Arduino base URL.
arduino = 'http://192.168.1.177/'

# Avoid using too much bandwidth, so don't check the channels continuously.
interval = 30 # scan rate in seconds

def show_license(title,year,author):
  print(title);
  print('Copyright (C) ' + year + ' ' + author);
  print('This program comes with ABSOLUTELY NO WARRANTY.');
  print('This is free software, and you are welcome to redistribute it');
  print('under certain conditions; see source code for details.');

# Function.
def extract_channel_title(html):
	i = html.find('<title>') # find beginning of channel title
	i += 9 # jump over title tag
	j = html[i:i+100].find('- YouTube') # find end of channel title
	j -= 2 # jump back over CRLF
	return html[i:i+j] # return channel name

# Function.
def extract_subscriber_count(html):
	# first try to find unique (hopefully) main tag
	i = html.find(tag)
	count = ''
	if i!=-1:
		len = 200
		html2 = html[i:i+len]
		# find second tag in substring
		i = html2.find('title=\"')
		if i!=-1:
			i += 7 # skip to start of subscriber count value
			len = 50
			html3 = html2[i:i+len].encode('utf-8') # convert string to bytes
			i = 0
			while html3[i]!=ord('"'):
				ch = html3[i] 
				if (ch>127): # skip long unicode characters
					if (ch>=192): i += 1 # at least a 2-byte code, skip byte
					if (ch>=224): i += 1 # at least a 3-byte code, skip byte
					if (ch>=240): i += 1 # 4-byte code, skip byte
					count += ',' # add thousands separator
				else:
					count += chr(ch) # add valid char to string
				i += 1 # next byte
	return count

# Function.
def youtube_channel_read(uri):
	# Get channel page.
	try:
		content = http.request('GET',uri)
	except urllib3.exceptions.HTTPError as e:
		# Encountered an error, print error message and quit.
		print('could not reach ' + uri + 'aborting...')
		return
		
	# Seve the YouTube page for offline analysis.
	#file = open("youtubeget.txt","w",encoding='utf-8') 
	#file.write(html) 
	#file.close()
	
	# Extract channel title and subscriber count from HTML page.
	html = content.data.decode('utf-8') # convert bytes to string
	title = extract_channel_title(html)
	count = extract_subscriber_count(html)
	# Display and log results.
	print(title + ': ' + count)
	log_file.write(',"' + count + '"')

	# Transmit channel title and subscriber count to the Arduino by means of a GET request.
	arduri = arduino + '"' + title + '"' + '+' + '"' + count + '"'
	try:
		# Send the GET request to the Arduino.
		content = http.request('GET',arduri,timeout=2.0) # local network, should respond quickly.
	except urllib3.exceptions.TimeoutError as e:
	#except (ConnectionError, TimeoutError) as e:
		print('Arduino is not responding')
		#print(e.code)
	except urllib3.exceptions.HTTPError as e:
		#print('Could not reach ' + arduri + ', error ' + str(e)) # print error
		print('Could not reach Arduino') # print error
		
	# For debugging, see if the Arduino understood our request.
	#html = content.data.decode('utf-8') # convert bytes to string
	#print(html)


# Allocate an HTTPS connection manager (or something like that).
http = urllib3.PoolManager(cert_reqs='CERT_REQUIRED',ca_certs=certifi.where())

# Reset time.
t = 0
# Open and initialize log file.
log_file = open(log_file_name,'w',newline='',encoding='utf-8') # UTF-8 required to preserve accents.
log_file.write('"Time' + '","' + user1 + '","' + user2 + '","' + user3 + '"\n')

# "Splash screen"
show_license('\nWIZnet IoT iOffload Contest','2019','Clemens Valens')
print('\nwaiting for data...')

while True:
	# Scan all the channels (and log the results).
	log_file.write('"' + str(t/3600) + '"')
	youtube_channel_read(uri1)
	youtube_channel_read(uri2)
	youtube_channel_read(uri3)
	log_file.write('\n')
	log_file.flush()
	# Sleep until next scan.
	time.sleep(interval)
	t += interval

# Hitting Ctrl-C (or an exception) will get you here. Clean up and get out.
log_file.close()
print('done')
