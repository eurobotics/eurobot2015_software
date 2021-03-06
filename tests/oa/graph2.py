import sys, re, math
import numpy as np
import matplotlib
import matplotlib.path as mpath
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
from matplotlib.patches import Arrow, Circle, Wedge, Polygon, Rectangle
from matplotlib.collections import PatchCollection
import popen2, random

Path = mpath.Path


def build_poly(ptlist):
    polydata = []
    polydata.append((Path.MOVETO, (ptlist[0])))
    for pt in ptlist[1:]:
        polydata.append((Path.LINETO, (pt)))
    polydata.append((Path.CLOSEPOLY, (ptlist[0])))
    codes, verts = zip(*polydata)
    poly = mpath.Path(verts, codes)
    x, y = zip(*poly.vertices)
    return x,y

def build_path(ptlist):
    polydata = []
    polydata.append((Path.MOVETO, (ptlist[0])))
    for pt in ptlist[1:]:
        polydata.append((Path.LINETO, (pt)))
    codes, verts = zip(*polydata)
    poly = mpath.Path(verts, codes)
    x, y = zip(*poly.vertices)
    return x,y

def draw_bottle(x, y):
    bottle = [ Rectangle((x-100, y), 200, 22*3) ]
    bottle += [ Rectangle((x-11, y), 22, -22*2) ]
    return bottle

def graph(filename, stx, sty, sta, enx, eny, op1x, op1y, op2x, op2y, robot_2nd_x, robot_2nd_y):
    cmd = "./main %d %d %d %d %d %d %d %d %d %d %d"%(stx, sty, sta, enx, eny, op1x, op1y, op2x, op2y, robot_2nd_x, robot_2nd_y)
    o,i = popen2.popen2(cmd)
    i.close()
    s = o.read(100000000)
    o.close()

    open(filename + ".txt", "w").write(s)

    if len(s) == 100000000:
        gloupix()

    fig = plt.figure()
    ax = fig.add_subplot(111)

    # total area limits
    x,y = build_poly([(0,0), (3000,0), (3000,2000), (0,2000)])
    ax.plot(x, y, 'k-')

    # play area limits
    """
    clerance = 230
    purple = 100
    red = 0
    x,y = build_poly([(400+clerance-10+purple,clerance), (3000-400-clerance+10-red,clerance),(3000-400-clerance+10-red,2000-44-clerance), (400+clerance-10+purple,2000-44-clerance)])
    #x,y = build_poly([(clerance,clerance), (3000-clerance,clerance),(3000-clerance,2000-44-clerance), (clerance,2000-44-clerance)])

    #x,y = build_poly([(240,240), (3000-240,240), (3000-240,2000-240-44), (240,2000-240-44)])
    ax.plot(x, y, 'c--')
    """

    # ground
    ground =[ Rectangle((0,0), 3000, 2000) ]

    # start areas
    start_area_yellow = [ Rectangle((0,800),400 , 400) ]
    start_area_red = [ Rectangle((3000-400, 800),400 , 400) ]

    # stairs
    stairs = [ Rectangle((967,2000-580), 1066,580 )]
    # platform
    platform = [ Rectangle((1200, 0), 600, 100) ]

    poly = None
    poly_wait_pts = 0
    start = None
    path = None
    patches = []
    for l in s.split("\n"):
        m = re.match("robot at: (-?\d+) (-?\d+) (-?\d+)", l)
        if m:
            x,y,a = (int(m.groups()[0]), int(m.groups()[1]), int(m.groups()[2]))
            path = [ (x,y) ]
            a_rad = (a * math.pi / 180.)
            dx = 150 * math.cos(a_rad)
            dy = 150 * math.sin(a_rad)
            patches += [ Circle((x, y), 50) ]
            patches += [ Arrow(x, y, dx, dy, 50) ]

        m = re.match("boundingbox at: (-?\d+) (-?\d+) (-?\d+) (-?\d+)", l)
        if m:
            x1,y1,x2,y2 = (int(m.groups()[0]), int(m.groups()[1]), int(m.groups()[2]), int(m.groups()[3]))
            x,y = build_poly([(x1,y1), (x2,y1), (x2,y2), (x1,y2)])
            ax.plot(x, y, 'c--')


            dst_x,dst_y = (int(m.groups()[2]), int(m.groups()[3]))
            patches += [ Circle((dst_x, dst_y), 50) ]

        m = re.match("oa_start_end_points\(\) \((-?\d+),(-?\d+)\) \((-?\d+),(-?\d+)\)", l)
        if m:
            dst_x,dst_y = (int(m.groups()[2]), int(m.groups()[3]))
            patches += [ Circle((dst_x, dst_y), 50) ]

        m = re.match("oa_new_poly\(size=(-?\d+)\)", l)
        if m:
            poly_wait_pts = int(m.groups()[0])
            poly = []

        m = re.match("opponent 1 at: (-?\d+) (-?\d+)", l)
        if m:
            poly_wait_pts = 4
            poly = []

        m = re.match("opponent 2 at: (-?\d+) (-?\d+)", l)
        if m:
            poly_wait_pts = 4
            poly = []

        m = re.match("robot 2nd at: (-?\d+) (-?\d+)", l)
        if m:
            poly_wait_pts = 4
            poly = []

        m = re.match("oa_poly_set_point\(\) \((-?\d+),(-?\d+)\)", l)
        if m:
            poly.append((int(m.groups()[0]), int(m.groups()[1])))
            poly_wait_pts -= 1
            if poly_wait_pts == 0:
                x,y = build_poly(poly)
                ax.plot(x, y, 'r-')

        m = re.match("GOTO (-?\d+),(-?\d+)", l)
        if m:
            path.append((int(m.groups()[0]), int(m.groups()[1])))

        m = re.match("With avoidance (-?\d+): x=(-?\d+) y=(-?\d+)", l)
        if m:
            path.append((int(m.groups()[1]), int(m.groups()[2])))

        m = re.match("nb_rays = (-?\d+)", l)
        if m:
            print((int(m.groups()[0])))


        p = PatchCollection(ground, cmap=matplotlib.cm.jet, alpha=0.1,  color='white')
        ax.add_collection(p)

        p = PatchCollection(start_area_red, cmap=matplotlib.cm.jet, alpha=1,  color='green')
        ax.add_collection(p)

        p = PatchCollection(start_area_yellow, cmap=matplotlib.cm.jet, alpha=1,  color='yellow')
        ax.add_collection(p)

        p = PatchCollection(stairs, cmap=matplotlib.cm.jet, alpha=1,  color='grey')
        ax.add_collection(p)

        p = PatchCollection(platform, cmap=matplotlib.cm.jet, alpha=1,  color='red')
        ax.add_collection(p)


    p = PatchCollection(patches, cmap=matplotlib.cm.jet, alpha=1)
    ax.add_collection(p)

    x,y = build_path(path)
    ax.plot(x, y, 'bo-')

    ax.grid()
    ax.set_xlim(-100, 3100)
    ax.set_ylim(-100, 2500)
    #ax.set_title('spline paths')
    #plt.show()
    fig.savefig(filename)

# args are: startx, starty, starta, endx, endy, opp1x, opp1y, opp2x, opp2y, robot_2nd_x, robot_2nd_y
#graph("single.png", 250, 250, 0, 2000, 1600, -1500, 0, -1500, 0)

"""
def random_target_xy():
   x = random.choice([1117, 1883, 2360])
   if x >= 1117 or x < 2360:
      y = random.choice([1600, 1400])
   else:
      y = random.choice([1600, 1000])
   return x,y

def random_robot_2nd_xy():
   x = random.choice([1600, 2000, 2500, 2600])
   if x >= 1500 or x <= 2500:
      y = random.choice([200, 500, 600])
   else:
      y = random.choice([600, 1000])
   return x,y

def random_opp_xy():
   x = random.choice([640, 1118, 1500, 1883, 2360])
   y = random.choice([1700+150, 1350])
   return x,y



# go in playground area
print("h_green1.png",2500, 500, 0, 2500, 1500, -1500, 0, -1500, 0, -1500, 0)
graph("h_green1.png",2500, 500, 0, 2500, 1500, -1500, 0, -1500, 0, -1500, 0)


print("h_green.png",1500, 500, 0, 2500, 1500, -1500, 0, -1500, 0, -1500, 0)
graph("h_green.png",1500, 500, 0, 2500, 1500, -1500, 0, -1500, 0, -1500, 0)
"""

# go in playground area
# args are:        startx, starty, starta, endx, endy, opp1x, opp1y, opp2x, opp2y, robot_2nd_x, robot_2nd_y
print("example.png")
graph("example.png", 400, 1600, 0, 2500, 300 ,1500, 1000, 1500, 1000, 1500, 1500)


