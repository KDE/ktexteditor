# What are these files?

These template files can be used for two things:

- As a reference when splitting frameworks: if you get strange CMake errors
  while building, check your framework structure matches with the templates.

- To create new frameworks: run `setup.sh <MyFramework> </destdir/myframework>`
  to copy the template files to `/destdir/myframework` and perform the
  appropriate renamings.

For this to be useful it has to be kept updated, I am going to do my best there,
but if you spot anything strange or outdated in there, please notify the
kde-frameworks-devel mailin list and/or fix it.

Note that the templates themselves are "buildable": you can create a build 
dir, point cmake to the "template" dir and "make install" a (useless) 
`libKTextEditor.so`.
