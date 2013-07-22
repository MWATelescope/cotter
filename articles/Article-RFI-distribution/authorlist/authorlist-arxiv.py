#!/usr/bin/env python
# Original version written by Ben Stappers on 19-09-2012


import os, sys, string, math
from optparse import OptionParser

def strip_end(text, suffix):
  if not text.endswith(suffix):
      return text
  return text[:-len(suffix)]

class author:
    def __init__(self, information):
        infolist = string.split(information,"\'")
        self.name = string.strip(infolist[1],"\'")
        self.institutes = string.split(infolist[-1])[:-1]
        self.printout = ""

parser = OptionParser()

parser.add_option("-a", "--authors", action="store", type="string", 
		  dest="authorlist",
		  help="Give the filename with the authors to be included on this paper. The format: \"'B~.W.~Stappers' jod yes\" where yes indicates that the author is to be included and jod indicates the institute, more than one institute can be used, yes must be the last thing on the line though")

parser.add_option("-i", "--institutes", action="store", type="string", 
		  dest="instlist",
		  help="Give the filename with the institutes to be included on this paper, Format is address followed by institute nickname, e.g. \"jod\".")

parser.add_option("-w", "--windows", action="store_true",dest="windows",
		  help="Give a windows format output")



(options, args) = parser.parse_args()
outlist=[]
authorfile = open(options.authorlist,'r')
for line in authorfile.readlines():
     if (string.split(line)[-1] == "yes"):
         outlist.append(author(line))

inpinstlist={}
instfile = open(options.instlist,'r')
for line in instfile.readlines():
    inpinstlist[string.split(line.rstrip())[-1]] = strip_end(line.rstrip(), string.split(line.rstrip())[-1]) 
#    inpinstlist.append(institute(line))

outinstlist={}
outinstlistchk=[]
instcnt=1
for author in outlist:
    for inst in author.institutes:
        if inst not in outinstlistchk:
            instinp = [ inst, instcnt ]
            outinstlist[inst] = instcnt
            outinstlistchk.append(inst)
            instcnt += 1

if (options.windows):

    print "----------------- Authors ---------------"

    for author in outlist:
        author.printout = str(author.name)
        first = 1
        for authorinst in author.institutes:
            if first==1:
                first = 0
            #else:
                #author.printout = author.printout + ', '
                #//author.printout = author.printout + str(outinstlist[authorinst])
        author.printout = author.printout
        print author.printout.replace('~',' ') + ',',

else:
    print "The other format still needs to be written!"


