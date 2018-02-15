FROM ubuntu:16.04

ADD cputest /cputest

ENTRYPOINT ["/cputest"]
