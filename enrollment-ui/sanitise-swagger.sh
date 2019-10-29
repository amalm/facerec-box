find ./src/lib/api/*.service.ts -type f -exec sed -i '' -e 's/rxjs\//rxjs/' {} \;
find ./src/lib/api/*.service.ts -type f -exec sed -i '' -e 's/formParams = formParams/formParams/' {} \;
find ./src/lib/api/*.service.ts -type f -exec sed -i '' -e 's/ || formParams//' {} \;