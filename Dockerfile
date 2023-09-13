FROM manjarolinux/base:latest

RUN pacman -Syu --noconfirm

SHELL [ "/bin/bash", "-c" ]

ENV SHELL=/bin/bash

WORKDIR /app

COPY /build/backend-cockfighting-api .
RUN chmod +x /app/backend-cockfighting-api

CMD /app/backend-cockfighting-api
