#!/usr/local/bin/python
from distorm3 import Decode,Decode16Bits,Decode64Bits,Decode32Bits
import sys
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import NoteSection
import re

def print_hex(data):
	print(" ".join("{:02x}".format(ord(c)) for c in data))

def process_file(filename):
    print 'Processing file: %s' % filename
    with open(filename, 'rb') as f:
        elffile = ELFFile(f)
        sect_n = '.text'
        text = elffile.get_section_by_name(sect_n) 
       	binary = text.data()
       	startOffset = text.header.sh_addr
       	pattern = re.compile(b"(\x74|\x75)[\x00-\xFF]")
       	fo = open("test.obj","wb")
       	changed = 0
       	for m in pattern.finditer(binary):
       		print_hex(m.group())
       		print hex(m.start())
       		binary = binary[:m.start()+changed] + '\xCC' + binary[m.start()+changed:]
       		changed += 1
       		print hex(m.start() + startOffset)
       	fo.write(binary)
       	print "Size of binary code is : 0x%x" % text.header.sh_size
       	print "Functions we are going to instrument : \n"
       	
       	l = Decode(startOffset,binary,Decode64Bits)
       	do_dis = False
       	linesDecoded = 0
     	for j in range (len(l)):
     		if l[j][3] == '55' and l[j+1][3] == '4889e5':
     			do_dis = True
     			print "sub_%x:" % l[j][0]
     			print
     		if do_dis == True:
       			print "0x%08x %-20s %s" % (l[j][0],  l[j][3],  l[j][2])
       			linesDecoded += 1
       		if (l[j][3] == "c3"):
       			print 
       			do_dis = False
       		elif (linesDecoded == 30):
       			print "..."
       			linesDecoded = 0
       			do_dis = False

# 554889e5

def main():
	if (len(sys.argv) < 2):
		print "Usage : ./sanitizer.py <elf binary>"
		sys.exit(-1)
	process_file(sys.argv[1])

main()

