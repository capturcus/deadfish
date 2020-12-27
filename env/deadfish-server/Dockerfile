FROM ubuntu:18.04

RUN mkdir /server

COPY deadfishserver /server
COPY *.so.* /usr/local/lib/
COPY levels /server/levels/

CMD ["bash", "-c", "/server/deadfishserver -p 63987 -l /server/levels/ship.bin --agones"]
