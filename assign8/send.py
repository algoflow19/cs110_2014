import random
import subprocess
from time import sleep

if __name__ == '__main__':
    while True:
        if random.randint(0, 1) == 0:
            send = 'alice'
            recv = 'bob'
        else:
            send = 'bob'
            recv = 'alice'

        amount = random.randint(1, 3)
        proc = subprocess.Popen(['../../coins', 'send', recv, str(amount)], cwd='servers/%s' % send)
        proc.wait()
        sleep(5)
