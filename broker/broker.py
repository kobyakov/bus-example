#! /usr/bin/python

import zmq

class Broker:

    class Error(Exception):
        pass

    def __init__(self, context):
        self.context = context or zmq.Context(1)

    def run(self):
        while True:
            self.step()

    def step(self):
        """@TODO"""
        return True

if __name__ == '__main__':
 
    b = Broker(zmq.Context(1))
    b.run()