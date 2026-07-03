VERSION=3.2.0

echo "Please enter the OS type (Linux / FreeBSD) : "
read OS

NAME=BBMan-full-$OS-${VERSION}
rm -rf ${NAME}
mkdir ${NAME}
gmake clean
gmake
strip obj/bbman
cp obj/bbman ${NAME}/
cp ../sites.dat ${NAME}/
cp -R ../theme ${NAME}/
cp -R ../mo ${NAME}/
upx ${NAME}/bbman
tar cfz ${NAME}.tgz ${NAME}

NAME=BBMan-nossh-$OS-${VERSION}
rm -rf ${NAME}
mkdir ${NAME}
gmake clean
gmake -f Makefile.nossh
strip obj/bbman
cp obj/bbman ${NAME}/
cp ../sites.dat ${NAME}/
cp -R ../theme ${NAME}/
cp -R ../mo ${NAME}/
upx ${NAME}/bbman
tar cfz ${NAME}.tgz ${NAME}
