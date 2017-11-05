#!/bin/bash

ps aux | grep FDSSTTest | grep -v grep | awk '{print $2}' | xargs -i -t kill -9 {}



