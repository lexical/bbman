VERSION=3.2.0

rm -rf unix/BBMan*
rm -rf unix/obj/*
rm -rf dev-c++/full/obj/*
rm -rf dev-c++/nossh/obj/*
cd ..
chmod -R 644 BBMan-src
tar cfz BBMan-src-${VERSION}.tgz BBMan-src
