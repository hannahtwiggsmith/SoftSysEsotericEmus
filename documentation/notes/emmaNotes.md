https://www.ittc.ku.edu/research/thesis/documents/sreenivas_penumarthy_thesis.pdf


This is a user-level threading library that someone made like five years ago. I don't know if it works yet, but it consists of two really long files without many comments:
https://github.com/ManojkumarMuralidharan/User-level-Thread-library

This seems like a fairly helpful stack overflow question, but probably for down the road. It deals specifically how to move between the stack that is associated with each thread:
http://stackoverflow.com/questions/9145321/implementing-a-user-level-threads-package

This seems like the Holy Grail. Someone without an enourmous amout fo C knowledge, creates their own user-level threading library, goes through a description of threading and other os-related stuff that seems good to know for this. Their libraries are very bare-bones and really only consist of creating threads, ending them, etc. which could be enough for us, it kind of depends on what we want to do and also what the teaching team thinks is enough. This person also implemented Kernal level threads, but we only want to make fibers/user-level threads so the first chunk of code in here isn't worth worrying over:
http://www.evanjones.ca/software/threading.html

This one is a user-level threading implemented by a professor. There isn't much explanation about what the program is doing, but there is some explanation about how to use it and pretty nice comments. Definitely fairly complex and in depth, also it's from 1998, so it's not made to run on our version of C or Linux, so there will definitely need to be some issue handling there is we want to base our stuff off of this: http://homepage.divms.uiowa.edu/~jones/opsys/threads/

This one is a little confusing and someone's personal project, but it seems like he attempted to explain the decsions he made. Also it is modeled after pthreads: https://machiry.wordpress.com/2012/01/02/mfiber-simple-user-land-thread-library/
