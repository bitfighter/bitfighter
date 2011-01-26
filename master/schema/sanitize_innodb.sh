#!/bin/bash
#
# remove InnoDB engine for unsupported DBs

sed -e 's/ENGINE=InnoDB //g' bitfighter.innoDB.sql > bitfighter.sql
