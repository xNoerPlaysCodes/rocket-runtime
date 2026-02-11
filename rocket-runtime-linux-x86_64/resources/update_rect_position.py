import time

cur_position = rocket.vec2f(0, 0)
last_time = time.time()
def update():
    global cur_position
    global last_time
    global time

    cur_position += rocket.vec2f(0.15, 0.15)
