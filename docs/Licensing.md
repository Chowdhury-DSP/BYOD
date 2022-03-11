# Licensing

BYOD is licensed under the General Public
License Version 3 (GPLv3), however, we are
asking contributors to sign a CLA so that
the plugin can also be distributed for iOS
under the BSD 3-clause license.

## Why a CLA?
The original intention was to license BYOD
under the GPLv3, however, the GPL is not
compatible with the iOS App Store rules 
(more information [here](https://www.fsf.org/blogs/licensing/more-about-the-app-store-gpl-enforcement)).
With that in mind, the CLA will allow the plugin
to be distributed under a license which is compatible
with the App Store rules (in this case BSD).

## What does the CLA mean?
BYOD is using the Canonical/Harmony 1.0 CLA
http://selector.harmonyagreements.org with
the following choices:

- You retain copyright to your code. We do not need you to assign copyright to us.
- You grant us a license to distribute your code under GPL3 or BSD; and your content under CC3 attribution

You can read the entire document [here](./BYOD-Individual-CLA.pdf).

## I want to contribute, how do I sign?
To agree to this document, please add your name
to the AUTHORS list in a git transaction where
you indicate in the git log message that you
consent to the CLA. You only need to do this once,
and you can do it in your first commit to the repo.

## I want to contribute, but I don't want to sign a CLA
If your contribution is to add a new guitar effect
processor (it is expected that most contributions
will be of this type), then that's perfectly fine!
Simply add a comment at the beginning of every file
that you add to the repo making note of your choice,
and when you add your name to the AUTHORS list, add
"[no CLA]" after your name. Following these steps will
alert us that you only want your code to be licensed
under the GPLv3, so we can make sure such code is
excluded from iOS builds.

If your contribution is something else that cannot be
trivialy excluded from iOS builds, then please create
a GitHub issue, and we can discuss the specific situation
in more detail.

## More details about the iOS distribution
Currently, the plan is to make the iOS distribution
available on the App Store for free. This plan will
not change without prior discussion with the authors
list.
