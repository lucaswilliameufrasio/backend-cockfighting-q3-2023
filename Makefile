setup:
	$ pip install conan && \ 
		sudo apt-get install git gcc g++ build-essential cmake
		# sudo apt-get install git gcc g++ libjsoncpp-dev uuid-dev zlib1g-dev build-essential cmake

	$ chmod +x ./scripts/build.sh
PHONY: setup

build-app:
	$ ./scripts/build.sh
PHONY: build-app

run:
	$ export $(cat .env | xargs) && ./build/backend-cockfighting-api
PHONY: run

build-image:
	# $ docker build -t lucaswilliameufrasio/backend-cockfighting-api --progress=plain .
	$ docker build --no-cache -t lucaswilliameufrasio/backend-cockfighting-api --progress=plain -f ./Dockerfile .
PHONY: build-image

start-database:
	$ docker compose -f docker-compose.dev.yml up -d postgres
PHONY: start-database

stop-all-compose-services:
	$ docker compose -f docker-compose.dev.yml down
	$ docker volume rm backend-cockfighintg-q3-2023_postgres_data
PHONY: stop-all-compose-services

run-container:
	$ docker run --rm --name backend-cockfighting-api --env-file=.env -p 9998:9998 lucaswilliameufrasio/backend-cockfighting-api
PHONY: run-container

stop-container:
	$ docker stop backend-cockfighting-api
PHONY: stop-container

push-image:
	$ docker push lucaswilliameufrasio/backend-cockfighting-api
PHONY: push-image

