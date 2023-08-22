FROM debian:12-slim

SHELL [ "/bin/bash", "-c" ]

ENV SHELL=/bin/bash

COPY /build/backend-cockfighting-api .

CMD /backend-cockfighting-api
