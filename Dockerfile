FROM debian:buster-slim

RUN apt-get update && apt-get install -yy \
	clang \
	gnuplot \
	make \
	sqlite3

# Get premake5
WORKDIR /root
ADD https://github.com/premake/premake-core/releases/download/v5.0.0-alpha13/premake-5.0.0-alpha13-linux.tar.gz premake.tar.gz
#RUN curl -O -J -L https://github.com/premake/premake-core/releases/download/v5.0.0-alpha13/premake-5.0.0-alpha13-linux.tar.gz
RUN tar -zxf premake.tar.gz
RUN mv premake5 /usr/local/bin/premake5
RUN rm premake.tar.gz

RUN echo ' \n\
if [ ! -d "/output" ]; then \n\
	echo "To get output, mount a directory at /output" \n\
	exit 1 \n\
fi \n\
 \n\
cp /work/graphs/output/* /output \n\
cp /work/graphs/*.csv /output \n\
' > /root/entrypoint.sh
RUN chmod u+x /root/entrypoint.sh

COPY . /work
WORKDIR /work/graphs
RUN make

CMD /root/entrypoint.sh
