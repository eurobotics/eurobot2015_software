#! /usr/bin/env python

import os,sys, re, math, termios,atexit
#import serial
from select import select
import cmd
from  matplotlib import pylab
from math import *

import popen2
from subprocess import Popen, PIPE
from fcntl import fcntl, F_GETFL, F_SETFL
from os import O_NONBLOCK, read

import struct

import shlex
import time
import math
import warnings
warnings.filterwarnings("ignore","tempnam",RuntimeWarning, __name__)

import numpy as np
import matplotlib.pyplot as plt

import logging
log = logging.getLogger("EuroboticsShell")
_handler = logging.StreamHandler()
_handler.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
log.addHandler(_handler)
log.setLevel(1)

EUROBOTICS_PATH=os.path.dirname(sys.argv[0])

#class SerialLogger:
#    def __init__(self, ser, filein, fileout=None):
#        self.ser = ser
#        self.filein = filein
#        self.fin = open(filein, "a", 0)
#        if fileout:
#            self.fileout = fileout
#            self.fout = open(fileout, "a", 0)
#        else:
#            self.fileout = filein
#            self.fout = self.fin
#    def fileno(self):
#        return self.ser.fileno()
#    def read(self, *args):
#        res = self.ser.read(*args)
#        self.fout.write(res)
#        return res
#    def write(self, s):
#        self.fout.write(s)
#        self.ser.write(s)

class Interp(cmd.Cmd):
    prompt = "Eurobotics > "

    def __init__(self):
        cmd.Cmd.__init__(self)

#        o, i = popen2.popen2("python ../maindspic/display.py")
#        o.close()
#        i.close()

        o, i = popen2.popen2("../secondary_robot/main H=1")
        o.close()
        i.close()

        self.p = Popen(['../maindspic/main', 'H=1'],
                  stdin = PIPE, stdout = PIPE, stderr = PIPE, shell = False)

        # set the O_NONBLOCK flag of p.stdout file descriptor:
        flags = fcntl(self.p.stdout, F_GETFL)
        fcntl(self.p.stdout, F_SETFL, flags | O_NONBLOCK)

        self.escape  = "\x01" # C-a
        self.quitraw = "\x02" # C-b
        self.serial_logging = False
        self.default_in_log_file = "/tmp/eurobotics.in.log"
        self.default_out_log_file = "/tmp/eurobotics.out.log"

    def do_quit(self, args):
        return True

#    def do_log(self, args):
#        """Activate serial logs.
#        log <filename>           logs input and output to <filename>
#        log <filein> <fileout>   logs input to <filein> and output to <fileout>
#        log                      logs to /tmp/eurobotics.log or the last used file"""

#        if self.serial_logging:
#            log.error("Already logging to %s and %s" % (self.ser.filein,
#                                                        self.ser.fileout))
#        else:
#            self.serial_logging = True
#            files = [os.path.expanduser(x) for x in args.split()]
#            if len(files) == 0:
#                files = [self.default_in_log_file, self.default_out_log_file]
#            elif len(files) == 1:
#                self.default_in_log_file = files[0]
#                self.default_out_log_file = None
#            elif len(files) == 2:
#                self.default_in_log_file = files[0]
#                self.default_out_log_file = files[1]
#            else:
#                print "Can't parse arguments"

#            self.ser = SerialLogger(self.ser, *files)
#            log.info("Starting serial logging to %s and %s" % (self.ser.filein,
#                                                               self.ser.fileout))


#    def do_unlog(self, args):
#        if self.serial_logging:
#            log.info("Stopping serial logging to %s and %s" % (self.ser.filein,
#                                                               self.ser.fileout))
#            self.ser = self.ser.ser
#            self.serial_logging = False
#        else:
#            log.error("No log to stop")


    def do_raw(self, args):
        "Switch to RAW mode"
        stdin = os.open("/dev/stdin",os.O_RDONLY)
        stdout = os.open("/dev/stdout",os.O_WRONLY)

        stdin_termios = termios.tcgetattr(stdin)
        raw_termios = stdin_termios[:]

        try:
            #log.info("Switching to RAW mode")

            # iflag
            raw_termios[0] &= ~(termios.IGNBRK | termios.BRKINT |
                                termios.PARMRK | termios.ISTRIP |
                                termios.INLCR | termios.IGNCR |
                                termios.ICRNL | termios.IXON)
            # oflag
            raw_termios[1] &= ~termios.OPOST;
            # cflag
            raw_termios[2] &= ~(termios.CSIZE | termios.PARENB);
            raw_termios[2] |= termios.CS8;
            # lflag
            raw_termios[3] &= ~(termios.ECHO | termios.ECHONL |
                                termios.ICANON | termios.ISIG |
                                termios.IEXTEN);

            termios.tcsetattr(stdin, termios.TCSADRAIN, raw_termios)

            mode = "normal"
            while True:
                ins,outs,errs=select([stdin,self.p.stdout],[],[])
                for x in ins:
                    if x == stdin:
                        c = os.read(stdin,1)
                        if mode  == "escape":
                            mode =="normal"
                            if c == self.escape:
                                self.p.stdin.write(self.escape)
                            elif c == self.quitraw:
                                return
                            else:
                                self.p.stdin.write(self.escape)
                                self.p.stdin.write(c)
                        else:
                            if c == self.escape:
                                mode = "escape"
                            else:
                                self.p.stdin.write(c)
                    elif x == self.p.stdout:
						c = self.p.stdout.read()						
						if c:						
							os.write(stdout,c)
        finally:
            termios.tcsetattr(stdin, termios.TCSADRAIN, stdin_termios)
            #log.info("Back to normal mode")

    def do_centrifugal(self, args):
        try:
            sa, sd, aa, ad 
        except:
            print "args: speed_a, speed_d, acc_a, acc_d"
            return
        print sa, sd, aa, ad
#        time.sleep(10)
#        self.fout.write("traj_speed angle %d\n"%(sa))
#        time.sleep(0.1)
#        self.fout.write("traj_speed distance %d\n"%(sd))
#        time.sleep(0.1)
#        self.fout.write("traj_acc angle %d\n"%(aa))
#        time.sleep(0.1)
#        self.fout.write("traj_acc distance %d\n"%(ad))
#        time.sleep(0.1)
#        self.fout.write("goto da_rel 800 180\n")
#        time.sleep(3)
##        self.fout.flushInput()
#        self.fout.write("position show\n")
#        time.sleep(1)
#        print self.fin.read()

    def do_cs_tune(self, args):
        try:
            name, cons, gain_p, gain_i, gain_d = [x for x in shlex.shlex(args)]
        except:
            print "args: consigne, gain_p, gain_i, gain_d"
            return

        cons = int(cons)
        gain_p = int(gain_p)
        gain_i = int(gain_i)
        gain_d = int(gain_d)
        print name, cons, gain_p, gain_i, gain_d

        self.p.stdin.write("log type cs on\n")
        self.p.stdin.write("position set 1500 1000 0\n")

        if name == "distance":  
            self.p.stdin.write("gain distance %d %d %d\n"%(gain_p, gain_i, gain_d))        
            self.p.stdin.write("goto d_rel %d\n"%(cons))
        elif name == "angle":  
            self.p.stdin.write("gain angle %d %d %d\n"%(gain_p, gain_i, gain_d))          
            self.p.stdin.write("goto a_rel %d\n"%(cons))
        else:
            print "unknow cs name"
            return

        time.sleep(0.2)

        TS = 5
        i = 0
        t = np.zeros(0)
        cons = f_cons = err = feedback = out = np.zeros(0)
        v_cons = v_feedback = np.zeros(1) 
        a_cons = a_feedback = a_cons = a_feedback = np.zeros(2)

        while True:
          time.sleep(0.01)
          line = self.p.stdout.readline()

          m = re.match("returned", line)
          if m:
            # end of data, plot it
            print line.rstrip()

            plt.figure(1)
            plt.subplot(311)
            plt.plot(t,v_cons, label="consigna")
            plt.plot(t,v_feedback, label="feedback")
            plt.ylabel('v(imp/Ts)')
            plt.grid(True)

            plt.subplot(312)
            plt.plot(t,a_cons, label="consigna")
            plt.plot(t,a_feedback, label="feedback")
            plt.ylabel('a(imp/Ts^2)')
            plt.grid(True)

            plt.subplot(313)
            plt.plot(t,out)
            plt.xlabel('t(s)')
            plt.ylabel('pwm(counts)')
            plt.grid(True)

            plt.figure(2)
            plt.subplot(211)
            plt.plot(t,f_cons, label="consigna")
            plt.plot(t,feedback, label="feedback")
            plt.ylabel('d(imp)')
            plt.grid(True)
            
            plt.subplot(212)
            plt.plot(t,err)
            plt.xlabel('t(s)')
            plt.ylabel('error(imps)')
            plt.grid(True)

            plt.show()
            break

          m = re.match("(-?\+?\d+).(-?\+?\d+): \((-?\+?\d+),(-?\+?\d+),(-?\+?\d+)\) "
                       "(\w+) cons= (-?\+?\d+) fcons= (-?\+?\d+) err= (-?\+?\d+) "
                       "in= (-?\+?\d+) out= (-?\+?\d+)", line)
          if m:
            #print m.groups()
            t = np.append(t, i*TS)
            cons = np.append(cons, int(m.groups()[6]))
            f_cons = np.append(f_cons, int(m.groups()[7]))
            err = np.append(err, int(m.groups()[8]))
            feedback = np.append(feedback, int(m.groups()[9]))
            out = np.append(out, int(m.groups()[10]))
            
            if i>0:
                v_cons = np.append(v_cons, f_cons[i] - f_cons[i-1])
                v_feedback = np.append(v_feedback, feedback[i] - feedback[i-1])

            if i>1:
                a_cons = np.append(a_cons, v_cons[i] - v_cons[i-1])
                a_feedback = np.append(a_feedback, v_feedback[i] - v_feedback[i-1])

            i += 1

          else:
            # print unknow strigs
            print line.rstrip()

#    def do_match(self, args):
#        m = re.match("(\w+).(\w+): \((\w+),(\w+),(\w+)\) (\w+) cons= (-?\+?\d+) fcons= (-?\+?\d+) err= (-?\+?\d+) in= (-?\+?\d+) out= (-?\+?\d+)", "7.300: (1199,479,0) distance cons= +835261 fcons= +835261 err= -00054 in= +835315 out= +00060")
#        if m:
#            print m.groups()
#        else:
#            print "not match"


    def do_position_show(self, args):
        time.sleep(0.1)
        self.fin.flush()
        time.sleep(0.1)
        #self.fout.write("position show\n")
        #time.sleep(1)
        #print self.fin.readline()

#    def do_tota(self, args):
#        print args
#        time.sleep(1)
#        self.fout.write("position set 0 0 0\n")
#        time.sleep(1)
#        self.fout.write("pwm s3(3C) 250\n")


if __name__ == "__main__":
    try:
        import readline,atexit
    except ImportError:
        pass
    else:
        histfile = os.path.join(os.environ["HOME"], ".eurobotics_history")
        atexit.register(readline.write_history_file, histfile)
        try:
            readline.read_history_file(histfile)
        except IOError:
            pass

    interp = Interp()
    while 1:
        try:
            interp.cmdloop()
        except KeyboardInterrupt:
            print
        except Exception,e:
            l = str(e).strip()
            if l:
                log.exception("%s" % l.splitlines()[-1])
            continue
        break
