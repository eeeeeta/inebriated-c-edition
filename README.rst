=====================
inebriated, C edition
=====================
*"everybody's favourite drunkard!"*

.. image:: http://i.imgur.com/GKLP6km.png
   :align: center

Huh?
----

This is a small toy project to implement a `Markov chains`_-based chatbot in C.
It takes inspiration from a version of this idea I previously made in `Node.js`_ and connected
to an IRC network - it soon produced hilarious results.

I still don't get it.
---------------------

STUPID SENTENCES! IN C! IN YOUR TERMINAL!!!111!!ONE!!111!

How?
----

Input sentence::

   "I like eating dogs and playing with cats".

This is then chopped up into two-word key-value pairs (which btw required A WHOLE DAMN FUNCTION to do in C, instead of .split() or something)::

   "I like" -> "eating dogs" -> "and playing" -> "with cats".

These pairs are stored in a database (the patented /dev/null Storage Device (tm)).

2nd input sentence::

 "I like making trains and playing with them"

Now, the database looks like this (except, knowing my coding, it probably doesn't)::

            >"eating dogs" ---|                 >"with them"
  [I like] -|                 |> "and playing" -|
            >"making trains" -|                 >"with cats"

To make a sentence, start at one of the 'sentence starters' (eg. "I like") and randomly select values. As more
and more sentences are added to the database, more funny opportunities arise!

(but of course, the values aren't just *selected*. they have a whole ALGOTHINGY go through them first!)

Your examples are weird.
------------------------

Go eat some chunky bacon.

Why?
----

For fun and profit!

Does it work?
-------------

Sort of. On my system, with my dataset (which contains a bunch of Bastard Operator from Hell excuses and dumps from
IRC chat logs), it generated the following gems:

..

  "Cosmic ray particles crashed through the livecd customization!"

  "Police are examining all internet packets in the channel insane."

  "Little hamster in running wheel had coronary; waiting for replacement to be recompiled"

How can I play with it?
-----------------------

Step I: install `tup`_.

Step II::

  $ tup
  $ ./markov

Step III: Listen to the lecture it gives you about how to use it. (i'll have you know, i spent a whole *minute* writing that!)

Step IV: ???

Step V: Profit!

It said something about networking.
-----------------------------------

There's an (alpha) networking bit that will eventually replace everything else. For now, though, it's
just fun to play with::

  $ ./markov [-p PORT] net

  [in another terminal]

  $ ./client [-h HOST] [-p PORT]


Bam. Crappy networking support, soon to be ported to a webpage near you.

Can I steal it?
---------------

You can do what you want with the code - it's `unlicensed`_. (I would like some attribution, though!)


.. _`markov chains`: "https://en.wikipedia.org/wiki/Markov_chain"
.. _`node.js`: "http://nodejs.org"
.. _`tup`: "http://gittup.org/tup/index.html"
.. _`unlicensed`: "http://unlicense.org/"

