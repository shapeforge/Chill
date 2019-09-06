# ChiLL, the node-based editor for IceSL

![ChiLL banner][banner]

## What is Chill?
Chill is a **node-based editor** specially tailored for [**IceSL**, the **Modeler and Slicer**](https://icesl.loria.fr).

Created by the members of the team behind IceSL, ChiLL is a tool which aims to provide a **Visual Programming** interface for IceSL.

As IceSL offers the possibility to directly create models using lua scripting, ChiLL will provide a way to create those modeling scripts using an approach based on Visual Programming. No need to code or learn a programming language!

## How to install ChiLL?
### On Windows
For Windows users, ChiLL is available through an **installer** (an .exe or .msi), or by retrieving the source code and **building the project**.

### On Linux
For Linux users, ChiLL is only available through retrieving the source code and **building the projet**.

Installers and pre-compiled versions of ChiLL will be provided on future releases.

## How to build ChiLL?
To build ChiLL, you will need the following components installed on your computer:

* Git
* CMAKE
* C++ 17 standard libraries
* Any IDE supporting C++ (on windows, Visual Studio is recomended)

>**Note**
>If you already have IceSL installed on your machine, you most likely already possess the required components tu run build Chill!

Once the required components installed, you can fetch the source code by cloning or dowloading a .zip of the source files.

```Shell
git clone https://github.com/shapeforge/Chill.git
```

You can now build the project for your IDE of choice using CMAKE, and then compile.

## How to Use ChiLL?
Once ChiLL is opened -- and IceSL alongside it -- you can access a menu containing the different node by doing a right-click.

![Nodes Menu][node_menu]

There you can select an input-node.

![Using an input-node][input_node]

You can now select an output-node and link the nodes by creating a link by clicking and dragging the link from the output of the first node to the input of the second node.

![Linking nodes][linking_nodes]

At launch time, ChiLL opens IceSL to produce a real-time preview of the model produced by the nodes. You can modify the parameters of any node interactively by typing or dragging values. The modifications are immediately reflected in the live preview of the object rendred in IceSL.

![Live preview with IceSL][live_preview]

Once the graph is complete, you can save it for later, or export a .lua file tailored for IceSL.

## How to contibute to ChiLL?
### New features or reworking the project
Create your own fork of the projet, and submit a [Pull Request](https://github.com/shapeforge/Chill/pulls). 
>**Note**
>When contributing using a Pull Request, don't hesitate to be precise in the description of your contribution, and don't forget to document what you did.
>It will greatly help us intergrate your changes! 

### Bug reports and feature requests
If you have a particular problem, or just an idea for a new feature, please fill an [Issue](https://github.com/shapeforge/Chill/issues), so we can discuss about it.
>**Note**
>Don't hesitate to be precise when creating an Issue. For bug reports, please join some scripts/files that reproduce the problem.
>Using an adapted label on your Issue will greatly help us sorting and process them.

# Credits
Main developpers:

* Jimmy Etienne [@JuDePom]
* Pierre Bedell [@Phazon54]
* Thibault Tricard [@ThibaultTricard]
* Cedric Zanni [@czanni]

Logos:

* Pierre-Alexandre Hugron

External libraries and tools used:

* [dear imgui](https://github.com/ocornut/imgui)
* [LibSL](https://github.com/sylefeb/LibSL)
* [lua5.1](https://www.lua.org/versions.html)
* [tinyfiledialogs](https://github.com/native-toolkit/tinyfiledialogs)

Special thanks to Sylvain Lefebvre [@sylefeb], father of IceSL, and Salim Perchy [@ysperchy], its main developper.

[//]: # (Ressources)
[banner]: ressources/images/chill_banner_wide_medium.png
[node_menu]: ressources/images/gifs/nodemenu.gif
[input_node]: ressources/images/gifs/inputnode.gif
[linking_nodes]: ressources/images/gifs/linknode.gif
[live_preview]: ressources/images/gifs/preview.gif
