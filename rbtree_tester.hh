/*
 * rbtree_tester.hh
 *
 *  Created on: Mar 9, 2017
 *      Author: simonm
 */

#ifndef RBTREE_TESTER_HH_
#define RBTREE_TESTER_HH_

#include "rbtree.hh"

#include <unistd.h>

#include <iostream>

class rbtree_tester
{
  public:

    void print()
    {
      print( tree.tree_root );
    }

    bool test_invariant()
    {
      return test_invariant( tree.tree_root ).first;
    }

    void clear()
    {
      tree.clear();
    }

    void populate()
    {
      srand( time( NULL ) );

      for( int i = 0; i < 1000; ++i )
      {
        int k = rand() % 1000 + 1;
        std::stringstream ss;
        ss << k;
        tree.insert( k, ss.str() );
      }

      for( int i = 0; i < 200; ++i )
      {
        int k = rand() % 1000 + 1;
        tree.erase( k );
      }
    }

  private:

    void print( const std::unique_ptr<rbtree<int, std::string>::node_t> &root, const std::string &indent = "" )
    {
      if( !root ) return;

      print( root->right, indent + "  " );
      std::string colour = root->colour ? "(R)" : "(B)";
      std::cout << indent << root->key << colour << std::endl;
      print( root->left, indent + "  " );
    }

    static std::pair<bool, int> test_invariant( const std::unique_ptr<rbtree<int, std::string>::node_t> &root )
    {
      // base case
      if( !root )
        return std::make_pair( true, 0 );

      int black = 0;
      if( root->colour == rbtree<int, std::string>::RED )
      {
        // RED node cannot have RED children
        if( ( root->left && root->left->colour == rbtree<int, std::string>::RED ) || ( root->right && root->right->colour == rbtree<int, std::string>::RED ) )
          return std::make_pair( false, -1 );
      }
      else
        black += 1;

      std::pair<bool, int> l = test_invariant( root->left );
      std::pair<bool, int> r = test_invariant( root->right );

      if( !l.first || !r.first )
        return std::make_pair( false, -1 );

      if( l.second != r.second )
        return std::make_pair( false, -1 );

      return std::make_pair( true, l.second + black );
    }

    rbtree<int, std::string> tree;
};

#endif /* RBTREE_TESTER_HH_ */
