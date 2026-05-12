# Installed Consumer Example

This example is configured only against an installed Mirakanai SDK through `find_package(Mirakanai CONFIG REQUIRED)`.

It intentionally avoids source-tree include paths so release validation can prove the exported `mirakana::` targets, installed headers, schemas, and AI manifest are usable by a clean consumer project.

