'''
Created on Feb 10, 2017

@author: simonm
'''

from numpy import linalg
from numpy import array

def GetVarName( node, path ):
    
    # converts a node name and a path (list of nodes)
    # e.g.: 'S3' and ['S1']
    # into a var name
    # e.g.: 'P(S3\S1)'
    
    varName = 'P(' + node
    if path :
        varName += '\\' + ';'.join( path )
    varName += ')'
    return varName

def VarToPath( var ):

    # converts var name like: P(S2\S3)
    # to a semicolon delimited path like: S3;S2;
    
    path = var[2:]
    path = path[:-1]
    if '\\' in path:
        s = path.split( '\\' )
        path = s[1] + ';' + s[0]
    path += ';'
    return path

def PrintTree( tree ):    
    indent = ''
    for lvl in tree[1:]:
        for k in lvl:
            path  = k.rstrip( ';' ).split( ';' )
            print indent, GetVarName( path[-1], path[:-1] ), lvl[k]
        indent += '  '

def SolveEquasions( equasions, varNames ):
    
    a = [] # matrix of coeficients
    b = [] # "dependent variable" values
    
    for e in equasions:
        b.append( e[0] )
        coefficients = [0] * len( varNames )
        for c in e[1]:
            coefficients[varNames.index(c[0])] = c[1]
            pass
        a.append( coefficients )
    
    # use the least-squares method, because most 
    # likely we will end up with a singular matrix
    # and infinite number of solutions
    p = linalg.lstsq( array( a ), array( b ) )
    return dict( zip( varNames, p[0] ) )

def CalcNextLvl( tree, nodes ):
    # last level of our current tree
    leaves = tree[-1]
    # the next level under construction
    nextLvl = { }
    # the list of equasions we will need to solve
    # e.g. for a system of equasions like:
    #      x + y = 10
    #      x - y = 2
    # equasions would hold following values:
    #    [( 10, [( x, 1.0 ), ( y, 1.0 )]), ( 2, [( x, 1.0 ), ( y, -1.0 )] )]
    equasions = []
    # the names of our variables
    varNames = []
    
    # loop over the leaves, and for 
    # each leave add new children
    for key in leaves:
        # the key by convention takes values like S1;S2;S3;
        path  = key.rstrip( ';' ).split( ';' ) if key else []
        # r is a set of all servers that were
        # not yet visited (on this path)
        r = set( nodes.keys() ) - set( path )
        if len( r ) < 2: continue

        # sum of probabilities for all new nodes that 
        # will be added to our particular leave is 1.0        
        eq = ( 1.0, [] )
        for node in r:
            # create next level
            k = key + node + ';'
            v = 0.0
            nextLvl[k] = v
            # create first part of the system of equasions
            varName = GetVarName( node, path )
            varNames.append( varName )
            # append another variable with its coefficient 
            # to the equasion
            eq[1].append( ( varName, 1.0 ) )
        # add the equasion to our system of equasions
        equasions.append( eq )

    # we will have one additional equasion per each data server
    eq = { k : [] for k in nodes.keys() }

    # for each node in next level calculate the probability
    # of ending up in this particular node
    for key in nextLvl:
        path  = key.rstrip( ';' ).split( ';' )
        varName = GetVarName( path[-1], path[:-1] )
        i = 1
        k = ''
        coefficient = 1.0
        # travers the tree and calc the product
        # of all known probabilities on our way
        for n in path[:-1]:
            k += n + ';'
            coefficient *= tree[i][k]
            i += 1
        # the probability is the variable times the coeficient
        # (the variable value will be obtained from the system 
        # of equasions we are producing right now)
        # we are agregating the variables per final node
        eq[path[-1]].append( ( varName, coefficient ) )
    
    # add the new equasions to our system of equasions
    # we agregated the (variable, coefficient) pairs per 
    # final node, so the sum of their products has to be
    # equal to the desired probability for the given node
    for key in eq:
        equasions.append( ( nodes[key], eq[key] ) )
        
    # solve the system of equasions, and update the next
    # level with respective probabilities
    probabilities = SolveEquasions( equasions, varNames )
    for v in probabilities:
        k = VarToPath( v )
        nextLvl[k] = probabilities[v]

    tree.append( nextLvl )
    

if __name__ == '__main__':

    # by convention the key is a string like: S1;S2;S3 (beging the path to given node) 
    tree = [{ '' : 1.0 }]
    
#     nodes = { 'S1' : 0.2, 'S2' : 0.4, 'S3' : 0.4 }
#     CalcNextLvl( tree, nodes )
#     CalcNextLvl( tree, nodes )
#     PrintTree( tree )

    nodes = { 'S1' : 0.1, 'S2' : 0.3, 'S3' : 0.3, 'S4' : 0.3 }
    CalcNextLvl( tree, nodes )
    CalcNextLvl( tree, nodes )
    CalcNextLvl( tree, nodes )
    PrintTree( tree )

