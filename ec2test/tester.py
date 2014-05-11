#!/usr/bin/python

import boto
import commands
import logging
import re
import sys
import time
import subprocess

from boto.ec2.connection import EC2Connection
from threading import Thread

if __name__ == "__main__":
    cloud = EC2Cloud()
    cloud.get_existing_instances()
    cloud.start_test("cd ~/libdsmu/src; git pull; make clean; make; ./matrixmultiply2 10.111.148.159 4444")

class EC2Instance():

    def __init__(self, i, key_location):
        self.instance = i
        self._key_location = key_location

    @property
    def private_ip(self):
        return self.instance.private_ip_address

    @property
    def public_ip(self):
        return self.instance.ip_address

    @property
    def public_dns_name(self):
        return self.instance.public_dns_name

    @property
    def key_location(self):
        return self._key_location

    @property
    def user_name(self):
        return "ubuntu"

    @property
    def state(self):
        return self.instance.state

    @property
    def name(self):
        return self.instance.tags['Name']

    def wait_for_state(self, state):
        while self.state != state:
            try:
                self.instance.update()
            except boto.exception.EC2ResponseError as e:
                if re.compile("InvalidInstanceID\.NotFound").search(str(e)):
                    time.sleep(10)
                else:
                    sys.stderr.write("Exception in wait for state\n")
                    sys.stderr.write(str(e) + "\n")
                pass
            time.sleep(1)


    def ssh_and_run_command(self, command, ssh_flags=""):
        d = "ssh %s -i %s -q -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null %s@%s \'%s\'" %\
            (ssh_flags, self.key_location, self.user_name, self.public_dns_name, command)
        subprocess.call(d, shell=True)
        #output = commands.getoutput(d)

        #return output

    def ssh_is_up(self):
        # Checks whether SSH is up on an instance.
        print "[%s] checking if ssh available" % self.name
        x = self.ssh_and_run_command("echo hi")
        print "[%s] x returned %s" % (self.name, x)
        return x == "hi"

    def begin(self, cmd, arg):
        command = cmd + " " + str(arg) + " 9;"
        self.ssh_and_run_command(command)
        """
        output = ""
        timeout = time.time() + 60

        while not re.compile("DONE").search(output):
            #command = cmd + " " + str(arg) + " 9;"
            command = cmd
            print command
            self.ssh_and_run_command(command+ " echo DONE")
            #print "[%s] " % self.name + str(output)
            time.sleep(1)
            if time.time() > timeout:
              break
        """

class EC2Cloud():

    def __init__(self):
        self.key_location = "~/.ssh/id_rsa.pub"
        self.key_pair = None
        self.key_name = "id_rsa.pub"
        self.security_group_name = None
        self.security_group = None # This is a security group object, which boto uses
        self.region = None
        self.instances = []

        self.key_pair = ("AKIAIUG2VHAF3BKOO7IQ","AYlLG5+QObC60HRbqZqxIEOlVIcMQ1I6bGvDYFSM") 
        self.security_group_name = "launch-wizard-1"
        self.region = "us-east-1"

        self.ec2_connection = None

    def get_existing_instances(self):
        self.ec2_connection = EC2Connection(self.key_pair[0], self.key_pair[1], region=boto.ec2.get_region(self.region))
        for r in self.ec2_connection.get_all_instances():
            for i in r.instances:
                if i.state == "running" and i.tags['Name'] not in ["1"]:
                    self.instances.append(EC2Instance(i, self.key_location))
        threads = list()
        for i, x in enumerate(self.instances):
            t = Thread(target=x.ssh_is_up)
            threads.append(t)
            t.start()
        for t in threads:
            t.join()

        print("Got %d existing instances" % len(self.instances))
        for inst in self.instances:
          print "Instance: " + inst.public_dns_name

    def start_test(self, cmd):
        threads = list()
        for i, x in enumerate(self.instances):
            t = Thread(target=x.begin, args=(cmd, x.name))
            threads.append(t)
            t.start()
        for t in threads:
            t.join()

