
# Open Shot Clock

Open Shot Clock is an Open Source and DIY Project aiming for minimizing the barriers of entry for shot-clock-aided sports like Boxlacrosse and Sixes and maximizing the pure sports experience.


## What´s Included?

- Design Documentation for Open Shot Clock Displays and Controller
    - CAD and PCB designs
    - 3d printing files
    - DIY Building Instructions Manual (german)
    - BOM for Displays and Controller (german)
    - User Manual (german)
    - Source Code for running Displays and Controller
- Project Wiki (still in build)



## Usage

For building your own Open Shot Clock you can find all documents/information in the documentation folder.

### Getting all the parts

Please go through the BOM to make shure you have all needed parts available. In the BOM we have included recommendation links where we recently purchased needed parts. Be aware that you can choose from different design features which you have to cover in your purchases. The developement is stil ongoing so some design features are not yet tested or even sufficiently developed, there could be a risk either in implementic them or even not.

In the current design 3d printing is requried. Make sure you print the necessary parts before starting to build or let them print by a printing service. Make sure the parts are printed with a head and impact resistant 3d printing material. You can find recommented materials in the BOM.

For the PCBs you can use the design files to order them at a PCB manufacturer like [PCBWay](https://www.pcbway.com/) or you can get in touch with us, there are still some prototyping PCBs left of the current version. In the near future the PCB design has to be adepted to the new Heltec Lora 32 version 3.0, up to now the new version is not supported yet)

### List of optional design features:

- Display Temperature Managment (up to now we don´t know if a temperature management is necessary, to find out we integrated a temp sensor socket on the display pcbs, tests are still in planning)

  - No Cooling
  - Fan grids on the back for passiv cooling (default)
  - Active Fan with fan grids on the back (fan can be installed behind one fan grid and operated by one mosfet output, this feature is not yet included in the sourc code)

- Batteries (you can find different battery options in the BOM from which you can choose)
- Accessories (you can find different accessory options in the BOM from which you can choose)
- Type-Plate customization (for having your own type-plate text you can get in touch with us, we can change the text for you)
- to be continued

### Assembling

When you have all parts available please follow the DIY Building Instructions Manual for assembling your Open Shot Clocks. Some parts like the base-, front- and back-plates need to be modified before they can be assembled. You can do the needed modification by using the drawings of these parts or you can get in touch with us, there are still some back plates left in our stock.


### How to set up the firmware

You can find the source code for the controller and the displays in the code folder. Pleas just copy this repo to your VS Code with Plaformio extansion and open both as projects. Fill in your prefered wifi credentials for setting up local device wifi networks to change settings.

- flash controller
  - SPIFFS
  - source code
- flash displays
  - source code
 

## What´s left to do?

- Tackle issues
- Build up Wiki with design insights, project history and roadmap
- Source code refactoring
- Design and test mdf display case as cost reduction opportunity
- pcb design to be ported to another open source platform and integration of heltc lora 32 v3.0 support
- Support first builders and gather feedback
- Offer english version of documents (if sufficiently demanded)
- to be continued


## How to contribute

Feel free to customize your Open Shot Clock to your needs. For this purpose you can find the design files in the cad and pcb folders and the source code in the code folder. If you have ideas how to improve the Open Shot Clock and you need support for the integration or you want to share the benefits with the community please get in touch with us.


## Authors

- [@ChangeD20](https://github.com/ChangeD20)
- [@devdrik](https://github.com/devdrik)


## License

[CERN OHL v2 Permissive](https://choosealicense.com/licenses/cern-ohl-p-2.0/)
