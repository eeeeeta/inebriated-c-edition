=====================
inebriated, C edition
=====================
*"everybody's favourite drunkard!"*

Huh?
----

This is a small toy project to implement a `Markov chains`_-based chatbot in C.
It takes inspiration from a version of this idea I previously made in `Node.js`_ and connected
to an IRC network - it soon produced hilarious results.

How?
----

The entire principle of this chatbot is to generate *plausibly hilarious sentences*, in that they
contain some aspect of grammatical correctness while still being nonsense. This is achieved in the following
fashion:

Input sentence::

   "I like eating dogs and playing with cats".

This is then chopped up into two-word key-value pairs::

   "I like" -> "eating dogs" -> "and playing" -> "with cats".

These pairs are stored in a database.
2nd input sentence::

 "I like making trains and playing with them"

Now, the database looks like this::

            >"eating dogs" ---|                 >"with them"
  [I like] -|                 |> "and playing" -|
            >"making trains" -|                 >"with cats"

To make a sentence, start at one of the 'sentence starters' (eg. "I like") and randomly select values. As more
and more sentences are added to the database, more funny opportunities arise!

Why?
----

For fun and profit!

Does it work?
-------------

Sort of.

How can I play with it?
-----------------------

First, install `tup`_. Then::

  $ tup init
  $ tup upd
  $ touch markov_keys.mkdb
  $ ./markov {infile, gen, input}

It's in beta. Be gentle. The *infile* option will read lines of sentences from a file named *infile.txt* in the current
directory, and the *input* option will let you do input sentences on the command line.

After that, use the *gen* command to try and generate a sentence.

Can I steal it?
---------------

You can do what you want with the code - it's `unlicensed`_. (I would like some attribution, though!)

.. _`markov chains`: "https://en.wikipedia.org/wiki/Markov_chain"
.. _`node.js`: "http://nodejs.org"
.. _`tup`: "http://gittup.org/tup/index.html"
.. _`unlicensed`: "http://unlicense.org/"

