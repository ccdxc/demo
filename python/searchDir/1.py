# check dict to remove line that less then 8

import os

def mkdir(path):

    import os
 

    path=path.strip()

    path=path.rstrip("\\")
 

    isExists=os.path.exists(path)
 

    if not isExists:

        os.makedirs(path) 
 
        print path+' success'
        return True
    else:

        print path+' has exist'
        return False

def writeNew(old,new,newdir):
    print old, ' '  , new
    f=open(old,"r")
    mkdir(newdir)
    f2=open(new,"w")
    for line in f:
        if len(line) >=8:
            f2.write(line)
    f.close();
    f2.close();
        

rootdir= "d:\dict3"
newdir= "d:\dict3_new"

for root, dirs, files in os.walk(rootdir):
    for file in files:
        writeNew(os.path.join(root,file),os.path.join(root.replace(rootdir,newdir),file),root.replace(rootdir,newdir))
