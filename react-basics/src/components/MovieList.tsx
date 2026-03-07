import { useEffect, useState } from "react";

interface MovieCardProps{
  title: string;
  rating: number;
}

function MovieCard(props: MovieCardProps){
  const [hasLiked, setHasLiked] = useState(false);
  const [clickCount, setClickCount] = useState(0);

  // useEffect takes two parameters: a function and a dependency array
  // no array: the function runs AFTER every render
  // empty array: the function runs once on mount
  // array with variables: the functions run when any of the variables change
  useEffect(() => {
    console.log(`${props.title} has been liked: ${hasLiked}`);
  }, [hasLiked]);

  const toggleLike = () => { setHasLiked(!hasLiked) }

  return(
    <div className="movie-card" onClick={() => {setClickCount(clickCount + 1)}}>
      <h2>{props.title} {clickCount ? '- ' + clickCount : null}</h2>
      <div style={{display: "flex"}}>
        <div>rating: {props.rating}</div>
        <div className={hasLiked ? 'like-button-liked' : 'like-button'} onClick={
          toggleLike
        }>{hasLiked ? 'Remove' : 'Like'}</div>
      </div>
    </div>
  )
}

function MovieList(){
  return(
    <div style={{display: "flex"}}> {/* Goofy syntax for in-document CSS styles. Flex puts them in a ROW */}
      <MovieCard title={'Star Wars'} rating={8}/>
      <MovieCard title={'Whiplash'} rating={8}/>
      <MovieCard title={'Ice Age'} rating={7}/>
    </div>
  )
}

export default MovieList;