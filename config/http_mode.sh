#!/bin/bash
conf_list=$(ls http_node*.yaml)
for cfg in ${conf_list[@]}; do
  echo "xxxx $cfg"
  new_name=${cfg:5}
  echo "xxxx new $new_name"
  cp -f $cfg $new_name
done
