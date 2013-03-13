#!/usr/bin/env python

from Tkinter import *
#from ttk import *
import tkFont
import Tix
import tkMessageBox

import socket
import time


"""
Protocol:

MAIN:
- messages should be send in lines (ended with \r\n)
- status should be send in following syntax *<status>*
- comments (additional information) should be send in following syntax ***<comment>***
- always wait for response
- empty or incorrect messages should be ignored

STATUS
- OK: message is correct processed
- ERROR: message could not be read

CONNECT
- send empty message to clear possible open buffers
- send *HELLO* status
- wait for *HELLO* status response

DISCONNECT
- send close message

"""
class NoProtocol:
	def __init__(self, parwin = None):
		self._win = parwin
	
	def send(self, msg, args = [], timeout = None):
		return

	def getComments(self):
		return []

	def connect(self):
		return

	def disconnect(self, mode = True, direct = False):
		if self._win != None and not direct:
			self._win.disconnect()

class Protocol:
	def __init__(self, ip, port, parwin = None):
		self._win = parwin
		self._ip = ip
		self._port = port
		self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self._connected = False
		self._comments = []

	def send(self, msg, args = [], timeout = 5):
		if self._connected == False: return False
		
		st = msg;
		for arg in args:
			st += " " + str(arg)
		#print st
		self._sock.send(st+'\r\n');
		while 1:
			self._sock.settimeout(timeout)
			data = ""
			try:
				data = self._sock.recv(1024)
			except:
				self.disconnect(False)
				return ""
				
			self._sock.settimeout(None)
			if data[:3] == "***": 
				if len(self._comments) > 100: self._comments.pop()
				self._comments.append(data[:-2])
				continue
			elif data[:1] == "*":
				if data.find("*OK*") != -1: return True
				else: return False
			else:
				return data[:-2]
	
	def getComments(self):
		return self._comments;
		
	def connect(self):
		if self._connected: return True
		try:
			self._sock.settimeout(3)
			self._sock.connect((self._ip, self._port))
			#self._sock.send("\r\n") # empty line to reinitialize
			self._sock.send("*HELLO*\r\n")
			data = self._sock.recv(1024)
			self._sock.settimeout(None)
			#print data
			if data.find("*HELLO*") == -1:
				self.disconnect(False)
				return False
				
			self._connected = True
		except socket.error:
			self._sock.settimeout(None)
			self._sock.close();
			return False
		return True
		
	def disconnect(self, mode = True, direct = False):
		if self._win != None and not direct:
			self._win.disconnect(True)
			
		if not self._connected: return
		if mode: self.send("close", timeout=5)
		self._sock.close();
		self._connected = False

class Move(Frame):
	def __init__(self, parent, tab):
		# configure fonts
		self._normal_font = tkFont.Font(family="Helvetica", size=10);
		self._big_font = tkFont.Font(family="Helvetica", size=10, weight=tkFont.BOLD);
		self._italic_font = tkFont.Font(family="Helvetica", size=10, slant=tkFont.ITALIC);
		
		Frame.__init__(self, tab)
		self._par = parent
		
		self.pack(side=Tix.LEFT, padx=2, pady=2, fill=Tix.BOTH, expand=1)
		
		Label(self, text="Velocity (%)", font=self._big_font).grid(row=0, column=0, sticky=NW, padx=4);
		self.vel = Tix.Control(self, state=DISABLED, value=60, llimit=0, ulimit=100)
		self.vel.grid(row=0, column=1, pady=2, sticky=NW)
		Label(self, text="Time (s)", font=self._big_font).grid(row=1, column=0, sticky=NW, padx=4);
		self.time = Tix.Control(self, value=1, llimit=0)
		self.time.grid(row=1, column=1, pady=2, sticky=NW)
		
		self._bup = Button(self, text="Up", width=8, command=self.up)
		self._bup.grid(row=0, column=4, sticky=NSEW)
		self._lup = Button(self, text="Left", width=8, command=self.left)
		self._lup.grid(row=1, column=3, sticky=NSEW)
		
		self._rup = Button(self, text="Right", width=8, command=self.right)
		self._rup.grid(row=1, column=5, sticky=NSEW)
		self._dup = Button(self, text="Down", command=self.down)
		self._dup.grid(row=1, column=4, sticky=NSEW)
		
		self.grid_columnconfigure(2, minsize=30);
		
	def activate(self):
		self._bup["state"] = self._rup["state"] = self._lup["state"] = self._dup["state"] = DISABLED
		
	def reset(self):
		self._bup["state"] = self._rup["state"] = self._lup["state"] = self._dup["state"] = NORMAL
		
	def up(self):
		if self._par.prot.send("forward", self.time["value"]):
			self.activate()
			self.after(int(self.time["value"])*1000, self.reset)
	def down(self):
		if self._par.prot.send("backward", self.time["value"]):
			self.activate()
			self.after(int(self.time["value"])*1000, self.reset)
		
	def left(self):
		if self._par.prot.send("turnLeft", self.time["value"]):
			self.activate()
			self.after(int(self.time["value"])*1000, self.reset)
	
	def right(self):
		if self._par.prot.send("turnRight", self.time["value"]):
			self.activate()
			self.after(int(self.time["value"])*1000, self.reset)

class Sensor(Frame):
	def __init__(self, parent, tab):
		# configure fonts
		self._normal_font = tkFont.Font(family="Helvetica", size=10);
		self._big_font = tkFont.Font(family="Helvetica", size=25, weight=tkFont.BOLD);
		self._italic_font = tkFont.Font(family="Helvetica", size=10, slant=tkFont.ITALIC);
		
		Frame.__init__(self, tab)
		self._par = parent

		self.pack(side=Tix.LEFT, padx=2, pady=2, fill=Tix.BOTH, expand=1)
		
		Label(self, text="Distance", font=self._big_font).grid(row=0, column=0, sticky=NW, padx=5, pady=5);
		self._dist = IntVar()
		self._read_dist = False
		Label(self, textvariable=self._dist, font=self._big_font, width = 10).grid(row=0, column=1, sticky=NW, padx=5, pady=5);
		self._dist_but = StringVar();
		self._dist_but.set("Read");
		Button(self, textvariable=self._dist_but, width=8, command=self.toggle_dist).grid(row=0, column=2, sticky=NSEW)
		
		Label(self, text="Heading", font=self._big_font).grid(row=1, column=0, sticky=NW, padx=5, pady=5);
		self._read_head = False
		self._head = IntVar()
		Label(self, textvariable=self._head, font=self._big_font, width = 10).grid(row=1, column=1, sticky=NW, padx=5, pady=5);
		self._head_but = StringVar();
		self._head_but.set("Read");
		Button(self, textvariable=self._head_but, width=8, command=self.toggle_head).grid(row=1, column=2, sticky=NSEW)
		
		Label(self, text="Acceleratie", font=self._big_font).grid(row=2, column=0, sticky=NW, padx=5, pady=5);
		self._read_speed = False
		self._speed = IntVar()
		Label(self, textvariable=self._head, font=self._big_font, width = 10).grid(row=2, column=1, sticky=NW, padx=5, pady=5);
		self._speed_but = StringVar();
		self._speed_but.set("Read");
		Button(self, textvariable=self._speed_but, width=8, command=self.toggle_head).grid(row=2, column=2, sticky=NSEW)
		
		self.after(500, self.read)
		
	def read(self):
		if self._read_dist:
			val = self._par.prot.send("readSonar");
			if val: self._dist.set((int(val)/8.0)*2.54)
		
		if self._read_head:
			val = self._par.prot.send("readHead");
			if val: self._head.set(int(val))
		if self._read_speed:
			val = self._par.prot.send("readSpeed");
			if val: self._head.set(int(val)*9.81)
		
		self.after(500, self.read)

	def toggle_dist(self):
		if self._read_dist and self._par.prot.send("sonar", [0]):
			self._read_dist = False;
			self._dist_but.set("Read")
		elif self._read_dist == False and self._par.prot.send("sonar", [1]):
			self._read_dist = True;
			self._dist_but.set("Stop reading")
			
	def toggle_head(self):
		if self._read_head and self._par.prot.send("heading", [0]):
			self._read_head = False;
			self._head_but.set("Read")
		elif self._read_head == False and self._par.prot.send("heading", [1]):
			self._read_head = True;
			self._head_but.set("Stop reading")

class Debug(Frame):
	def __init__(self, parent, tab):
		# configure fonts
		self._normal_font = tkFont.Font(family="Helvetica", size=10);
		self._big_font = tkFont.Font(family="Helvetica", size=10, weight=tkFont.BOLD);
		self._italic_font = tkFont.Font(family="Helvetica", size=10, slant=tkFont.ITALIC);
		
		Frame.__init__(self, tab)
		self._par = parent

		self.pack(side=Tix.LEFT, padx=2, pady=2, fill=Tix.BOTH, expand=1)
		
		Label(self, text="Version", font=self._big_font).grid(row=0, column=0, sticky=NW, padx=5, pady=5);
		self._version = StringVar()
		Label(self, textvariable=self._version, font=self._big_font).grid(row=0, column=1, columnspan=2, sticky=NW, padx=5, pady=5);
		
		Label(self, text="Light", font=self._big_font).grid(row=1, column=0, sticky=NW, padx=5, pady=5);
		self._light = Tix.Control(self, value=1, llimit=1, ulimit=2)
		self._light.grid(row=1, column=1, sticky=NW, padx=5, pady=5)
		Button(self, text="Toggle", width=8, command=self.enable_light).grid(row=1, column=2, sticky=NSEW)
		
	def enable_light(self):
		self._par.prot.send("light", [self._light["value"]])

class GUI(Frame):
	def __init__(self, p):
		Frame.__init__(self, p)
		self._root = p
		
		self._ip = StringVar()
		# DEBUG
		#self._ip.set("192.168.0.103")
		self._ip.set("192.168.178.26")
		self._port = IntVar()
		self._port.set(80)
		self.prot = NoProtocol(self)
		self.dialog = None
		
		self.pack()
		self.create()
	
	def create(self):
		# configure fonts
		self._normal_font = tkFont.Font(family="Helvetica", size=10);
		self._big_font = tkFont.Font(family="Helvetica", size=10, weight=tkFont.BOLD);
		self._italic_font = tkFont.Font(family="Helvetica", size=10, slant=tkFont.ITALIC);
		
		# create root window
		self._root.wm_title("Robot");
		#self._root.geometry("%dx%d" % (440, 400));
		self._root.resizable(0,0);
		
		# create main fields
		self._conn = Button(self, text="Connect", font=self._normal_font,command=self.connect)
		self._conn.grid(row=0, column=0, sticky=NSEW, padx=4, pady=10);
		self._disc = Button(self, text="Disconnect", font=self._normal_font, state=DISABLED, command=self.disconnect)
		self._disc.grid(row=0, column=1, padx=4, pady=10, sticky=NSEW);
		self._rec = Button(self, text="Reconnect", font=self._normal_font, state=DISABLED, command=self.connect_finalize)
		self._rec.grid(row=0, column=2, padx=4, pady=10, sticky=NSEW);
		
		self._command = StringVar();
		self._resp = StringVar();
		self._resp.set("");
		Label(self, text="Command", font=self._big_font).grid(row=1, column=0, sticky=NW, padx=4);
		self._entry = Entry(self, textvariable=self._command, width=30, font=self._normal_font);
		self._entry.grid(row=1, column=1, columnspan=2, sticky=NW);
		Label(self, textvariable=self._resp, font=self._big_font).grid(row=1, column=3, sticky=NW, padx=4);
		
		# create notebooks
		self.nb = Tix.NoteBook(self, name='nb', ipadx=6, ipady=6)

		self.nb.add('move', label="Move", underline=0)
		self.nb.add('sensor', label="Sensor", underline=0)
		self.nb.add('debug', label="Debug", underline=0)

		self.nb.grid(row=2, column=0, columnspan=4, sticky=NSEW, pady=5, padx=5)
		
		# create subs
		self._debug = Debug(self, self.nb.debug)
		Sensor(self, self.nb.sensor)
		Move(self, self.nb.move)
		
		# configure columns and rows
		cols, rows = self.grid_size();
		for i in range(cols):
			self.grid_columnconfigure(i, pad=5, minsize=100);
		for i in range(rows):
			self.grid_rowconfigure(i, pad=5);
	
	def connect(self):
		self.dialog = Toplevel(self)
		self.dialog.wm_title("Connect")
		self.dialog.transient(self)
		self.dialog.grab_set()
		self.dialog.resizable(0,0)
		
		Label(self.dialog, text="IP", font=self._big_font).grid(row=0, column=0, sticky=NW, padx=5, pady=5)
		Entry(self.dialog, textvariable=self._ip, width=15, font=self._normal_font).grid(row=1, column=0, sticky=NW, padx=5, pady=5)
		Label(self.dialog, text="Port", font=self._big_font).grid(row=0, column=1, sticky=NW, padx=5, pady=5)
		Entry(self.dialog, textvariable=self._port, width=5, font=self._normal_font).grid(row=1, column=1, sticky=NW, padx=5, pady=5)
		self.dialog._but = Button(self.dialog, text="Connect", width=10, font=self._normal_font,command=self.connect_finalize)
		self.dialog._but.grid(row=0, column=3, rowspan=2, sticky=NSEW, padx=5, pady=5)
		
	def connect_finalize(self):
		self.prot.disconnect()
		self.prot = Protocol(self._ip.get(), self._port.get(), self)
			
		if self.prot.connect() == False:
			tkMessageBox.showerror("Connection failed", "There was a problem connecting to the robot, are you're the IP-adress and port are correct? If this problem remains, please restart the robot.")
			self.prot = NoProtocol(self)
		else:
			self._debug._version.set(self.prot.send("version"))
			if self.dialog != None: 
				self.dialog.destroy()
				self.dialog = None
				
			self._conn["state"] = DISABLED
			self._rec["state"] = DISABLED
			self._disc["state"] = NORMAL
		
	def disconnect(self, tp = False):
		if not tp: self.prot.disconnect(mode = tp, direct = True)
		self.prot = NoProtocol(self)
		self._conn["state"] = NORMAL
		self._disc["state"] = DISABLED
		self._rec["state"] = NORMAL
		
def main():		
	# open root window
	root = Tix.Tk()
	
	# start applicatie
	gui = GUI(root)
	gui.mainloop()
	

if __name__ == "__main__":
	main()
